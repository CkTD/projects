#include <gflags/gflags.h>
#include <bthread/bthread.h>
#include <brpc/channel.h>
#include <brpc/controller.h>
#include <braft/raft.h>
#include <braft/util.h>
#include <braft/route_table.h>
#include "dlevel.pb.h"
#include <ctime>
#include <cstdlib>

DEFINE_bool(use_bthread, false, "Use bthread to send requests");
DEFINE_int64(added_by, 1, "Num added to each peer");
DEFINE_int32(thread_num, 1, "Number of threads sending requests");
DEFINE_int32(timeout_ms, 1000, "Timeout for each request");
DEFINE_string(conf, "", "Configuration of the raft group");
DEFINE_string(group, "DLEVEL", "Id of the replication group");

bvar::LatencyRecorder g_latency_recorder("counter_client");
using namespace dlevel;

class dlevel_client
{
  public:
    dlevel_client(std::string group, std::string conf, int timeout_ms)
    {
        if (braft::rtb::update_configuration(group, conf) != 0)
        {
            LOG(INFO) << "Fail to register configuration " << conf
                      << " of group " << group;
            exit(-1);
        }

        srand((unsigned)time(NULL));

        _group = group;
        _conf = conf;
        _timeout_ms = timeout_ms;
        _id = rand();
        LOG(INFO) << "group:" << _group;
        LOG(INFO) << "conf:" << _conf;
        LOG(INFO) << "Id: " << _id;
    }

    ~dlevel_client()
    {
        if (_id != 0)
        {
            LOG(INFO) << "Desctuctor called... close";
            Close();
        }
    }

  public:
    struct Result
    {
        bool error;        // is request failed  (e.g. network error)
        bool success;      // is operation success
        std::string value; // errstr if not success else the resule(only valid fot get)
    };

    struct Result Open(std::string db)
    {
        if (check_leader() != 0)
            return Result{true, false, ""};

        brpc::Channel channel;
        if (channel.Init(_leader.addr, NULL) != 0)
        {
            LOG(ERROR) << "Fail to init channel to " << _leader;
            return Result{true, false, ""};
        }

        DlevelService_Stub stub(&channel);

        brpc::Controller cntl;
        cntl.set_timeout_ms(_timeout_ms);

        DlevelRequest request;
        DlevelResponse response;
        request.set_key(db);
        request.set_actiontype(DlevelRequest_ActionType_OPEN);
        request.set_id(_id);

        stub.handler(&cntl, &request, &response, NULL);

        if (cntl.Failed())
        {
            LOG(WARNING) << "Fail to send request to " << _leader
                         << " : " << cntl.ErrorText();
            // Clear leadership since this RPC failed.
            braft::rtb::update_leader(_group, braft::PeerId());
            return Result{true, false, ""};
        }

        if (!response.success())
        {
            if (response.has_redirect())
            {
                LOG(WARNING) << "Fail to send request to " << _leader
                             << ", redirecting to "
                             << (response.has_redirect());
                // Update route table since we have redirect information
                braft::rtb::update_leader(_group, response.redirect());
                Open(db /*, value*/);
            }
            else
            {
                //*value = response.value();
                return Result{false, false, response.value()};
            }
        }
        g_latency_recorder << cntl.latency_us();

        LOG(INFO) << "Received response from " << _leader
                  << " OPEN Success: [" << response.success()
                  << "]. Key: [" << request.key()
                  << "]. Value: [" << response.value()
                  << "]. latency: [" << cntl.latency_us() << "]";
        // success
        return Result{false, true, "success"};
    }

    struct Result Close()
    {
        if (check_leader() != 0)
            return Result{true, false, ""};

        brpc::Channel channel;
        if (channel.Init(_leader.addr, NULL) != 0)
        {
            LOG(ERROR) << "Fail to init channel to " << _leader;
            return Result{true, false, ""};
        }

        DlevelService_Stub stub(&channel);

        brpc::Controller cntl;
        cntl.set_timeout_ms(_timeout_ms);

        DlevelRequest request;
        DlevelResponse response;
        request.set_key("...");
        request.set_actiontype(DlevelRequest_ActionType_CLOSE);
        request.set_id(_id);

        stub.handler(&cntl, &request, &response, NULL);

        if (cntl.Failed())
        {
            LOG(WARNING) << "Fail to send request to " << _leader
                         << " : " << cntl.ErrorText();
            // Clear leadership since this RPC failed.
            braft::rtb::update_leader(_group, braft::PeerId());
            return Result{true, false, ""};
        }

        if (!response.success())
        {
            if (response.has_redirect())
            {
                LOG(WARNING) << "Fail to send request to " << _leader
                             << ", redirecting to "
                             << (response.has_redirect());
                // Update route table since we have redirect information
                braft::rtb::update_leader(_group, response.redirect());
                Close();
            }
            else
            {
                //*value = response.value();
                return Result{false, false, response.value()};
            }
        }
        g_latency_recorder << cntl.latency_us();
        _id = 0;
        LOG(INFO) << "Received response from [" << _leader
                  << "]. Close Success: [" << response.success() << "]";
        // success
        return Result{false, true, "success"};
    }

    struct Result Put(std::string key, std::string value)
    {
        if (check_leader() != 0)
            return Result{true, false, ""};

        brpc::Channel channel;
        if (channel.Init(_leader.addr, NULL) != 0)
        {
            LOG(ERROR) << "Fail to init channel to " << _leader;
            return Result{true, false, ""};
        }

        DlevelService_Stub stub(&channel);

        brpc::Controller cntl;
        cntl.set_timeout_ms(_timeout_ms);

        DlevelRequest request;
        DlevelResponse response;
        request.set_key(key);
        request.set_value(value);
        request.set_actiontype(DlevelRequest_ActionType_PUT);
        request.set_id(_id);

        stub.handler(&cntl, &request, &response, NULL);

        if (cntl.Failed())
        {
            LOG(WARNING) << "Fail to send request to " << _leader
                         << " : " << cntl.ErrorText();
            // Clear leadership since this RPC failed.
            braft::rtb::update_leader(_group, braft::PeerId());
            return Result{true, false, ""};
        }

        if (!response.success())
        {
            if (response.has_redirect())
            {
                LOG(WARNING) << "Fail to send request to " << _leader
                             << ", redirecting to "
                             << (response.has_redirect());
                // Update route table since we have redirect information
                braft::rtb::update_leader(FLAGS_group, response.redirect());
                Put(key, value);
            }
            else
                return Result{false, false, response.value()};
        }
        g_latency_recorder << cntl.latency_us();

        LOG(INFO) << "Received response from [" << _leader
                  << "]. PUT Success: [" << response.success()
                  << "]. Key: [" << request.key()
                  << "]. Value: [" << response.value()
                  << "]. latency: [" << cntl.latency_us() << "]";

        return Result{false, true, response.value()};
    }

    struct Result Get(std::string key, std::string *value)
    {
        if (check_leader() != 0)
            return Result{true, false, ""};

        brpc::Channel channel;
        if (channel.Init(_leader.addr, NULL) != 0)
        {
            LOG(ERROR) << "Fail to init channel to " << _leader;
            return Result{true, false, ""};
        }

        DlevelService_Stub stub(&channel);

        brpc::Controller cntl;
        cntl.set_timeout_ms(_timeout_ms);

        DlevelRequest request;
        DlevelResponse response;
        request.set_key(key);
        request.set_actiontype(DlevelRequest_ActionType_GET);
        request.set_id(_id);

        stub.handler(&cntl, &request, &response, NULL);

        if (cntl.Failed())
        {
            LOG(WARNING) << "Fail to send request to " << _leader
                         << " : " << cntl.ErrorText();
            // Clear leadership since this RPC failed.
            braft::rtb::update_leader(_group, braft::PeerId());
            return Result{true, false, ""};
        }

        if (!response.success())
        {
            if (response.has_redirect())
            {
                LOG(WARNING) << "Fail to send request to " << _leader
                             << ", redirecting to "
                             << (response.has_redirect());
                // Update route table since we have redirect information
                braft::rtb::update_leader(_group, response.redirect());
                Get(key, value);
            }
            else
            {
                //*value = response.value();
                return Result{false, false, response.value()};
            }
        }
        *value = response.value();
        g_latency_recorder << cntl.latency_us();

        LOG(INFO) << "Received response from [" << _leader
                  << "]. GET Success: [" << response.success()
                  << "]. Key: [" << request.key()
                  << "]. Value: [" << response.value()
                  << "]. latency: [" << cntl.latency_us() << "]";
        // success
        return Result{false, true, response.value()};
    }

    struct Result Delete(std::string key)
    {
        if (check_leader() != 0)
            return Result{true, false, ""};

        brpc::Channel channel;
        if (channel.Init(_leader.addr, NULL) != 0)
        {
            LOG(ERROR) << "Fail to init channel to " << _leader;
            return Result{true, false, ""};
        }

        DlevelService_Stub stub(&channel);

        brpc::Controller cntl;
        cntl.set_timeout_ms(_timeout_ms);

        DlevelRequest request;
        DlevelResponse response;
        request.set_key(key);
        request.set_actiontype(DlevelRequest_ActionType_DELETE);
        request.set_id(_id);

        stub.handler(&cntl, &request, &response, NULL);

        if (cntl.Failed())
        {
            LOG(WARNING) << "Fail to send request to " << _leader
                         << " : " << cntl.ErrorText();
            // Clear leadership since this RPC failed.
            braft::rtb::update_leader(_group, braft::PeerId());
            return Result{true, false, ""};
        }

        if (!response.success())
        {
            if (response.has_redirect())
            {
                LOG(WARNING) << "Fail to send request to " << _leader
                             << ", redirecting to "
                             << (response.has_redirect());
                // Update route table since we have redirect information
                braft::rtb::update_leader(_group, response.redirect());
                Delete(key);
            }
            else
                return Result{false, false, response.value()};
        }
        g_latency_recorder << cntl.latency_us();

        // success
        LOG(INFO) << "Received response from [" << _leader
                  << "]. DELETE Success: [" << response.success()
                  << "]. Key: [ " << request.key()
                  << "]. Value: [" << response.value()
                  << "]. latency: [" << cntl.latency_us() << "]";

        return Result{false, true, response.value()};
    }

  private:
    int
    check_leader()
    {
        // Select leader of the target group from RouteTable
        if (braft::rtb::select_leader(_group, &_leader) != 0)
        {
            // Leader is unknown in RouteTable. Ask RouteTable to refresh leader
            // by sending RPCs.
            butil::Status st = braft::rtb::refresh_leader(_group, _timeout_ms);
            if (!st.ok())
            {
                // Not sure about the leader, sleep for a while and the ask again.
                LOG(WARNING) << "Fail to refresh_leader : " << st;
                return -1;
            }
            else
            {
                braft::rtb::select_leader(_group, &_leader);
                LOG(INFO) << "!!!!!! leader: [" + _leader.to_string() << "]";
                return 0;
            }
        }
        else
        {
            return 0;
        }
    }

  public:
    int32_t id()
    {
        return _id;
    }

  private:
    int32_t _id;
    braft::PeerId _leader;
    std::string _group;
    std::string _conf;
    int _timeout_ms;
};

int main(int argc, char *argv[])
{
    GFLAGS_NS::ParseCommandLineFlags(&argc, &argv, true);

    struct dlevel_client::Result result;
    int i;

    dlevel_client client(FLAGS_group, FLAGS_conf, FLAGS_timeout_ms);

    std::string k = "key_" + std::to_string(client.id()) + "_";
    std::string v = "value_" + std::to_string(client.id()) + "_";
    srand(time(NULL));
    std::string dbname = "db" + std::to_string(rand() % 2);


    // OPEN
    LOG(INFO) << "OPEN: [" << dbname << "].";
    result = client.Open(dbname);
    if (result.error)
        LOG(ERROR) << "OPEN: can't open db: "
                   << "db1";
    else
    {
        LOG(INFO) << "OPEN: "
                  << "issuccess: [" << result.success << "]. "
                  << "value: [" << result.value << "].";
    }

    i = 0;
    while (++i && !brpc::IsAskedToQuit())
    {
        // PUT
        LOG(INFO) << "PUT: [" << k + std::to_string(i) << ":" << v + std::to_string(i) << "].";
        result = client.Put(k + std::to_string(i), v + std::to_string(i));
        if (result.error)
            LOG(ERROR) << "PUT: can't put key: " << k + std::to_string(i)
                       << " see previous err message";
        else
            LOG(INFO) << "PUT: "
                      << "issuccess: [" << result.success << "]. "
                      << "value: [" << result.value << "].";
        sleep(1);

        // GET
        std::string value;
        LOG(INFO) << "GET: [" << k + std::to_string(i) << "]";
        result = client.Get(k + std::to_string(i), &value);
        if (result.error)
            LOG(ERROR) << "GET: can't get key: " << k + std::to_string(i)
                       << " see previous err message";
        else
            LOG(INFO) << "GET: "
                      << "issuccess: [" << result.success << "]. "
                      << "value: [" << value << "].";
        sleep(1);


        // DELETE
        if (i % 2 == 0)
            continue;
        LOG(INFO) << "DELETE: [" << k + std::to_string(i) << "]";
        result = client.Delete(k + std::to_string(i));
        if (result.error)
            LOG(ERROR) << "DELETE: can't delete key: " << k + std::to_string(i)
                       << " see previous err message";
        else
            LOG(INFO) << "DELETE: "
                      << "issuccess: [" << result.success << "]."
                      << "value: [" << result.value << "].";
        sleep(1);
    }

    LOG(INFO) << "CLOSE";
    client.Close();
}