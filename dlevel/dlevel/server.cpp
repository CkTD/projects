#include <gflags/gflags.h>       // DEFINE_*
#include <brpc/controller.h>     // brpc::Controller
#include <brpc/server.h>         // brpc::Server
#include <braft/raft.h>          // braft::Node braft::StateMachine
#include <braft/storage.h>       // braft::SnapshotWriter
#include <braft/util.h>          // braft::AsyncClosureGuard
#include <braft/protobuf_file.h> // braft::ProtoBufFile
#include "dlevel.pb.h"
#include "leveldb/db.h"

DEFINE_bool(check_term, true, "Check if the leader changed to another term");
DEFINE_bool(disable_cli, false, "Don't allow raft_cli access this node");
DEFINE_bool(log_applied_task, false, "Print notice log when a task is applied");
DEFINE_int32(election_timeout_ms, 1000,
             "Start election in such milliseconds if disconnect with the leader");
DEFINE_int32(port, 8100, "Listen port of this peer");
DEFINE_int32(snapshot_interval, 30, "Interval between each snapshot");
DEFINE_string(conf, "", "Initial configuration of the replication group");
DEFINE_string(data_path, "./data", "Path of data stored on");
DEFINE_string(leveldb_path, "./leveldb", "leveldb path");
DEFINE_string(group, "DLEVEL", "Id of the replication group");

namespace dlevel
{

// leveldb instance
class Level
{
  public:
    Level(std::string dbpath, int clientsnum = 1)
    {
        LOG(INFO) << "!!!!! LEVEL Constructor Called for db [" << dbpath << "]";
        this->_writeoptions = leveldb::WriteOptions();
        this->_readoptions = leveldb::ReadOptions();
        this->_clientsnum = clientsnum;
        this->_dbpath = dbpath;
        this->_options.create_if_missing = true;

        this->_status = leveldb::DB::Open(this->_options, this->_dbpath, &this->_db);
        if (!this->_status.ok())
        {
            LOG(ERROR) << "can't open db" << this->_status.ToString();
            throw "Can't Open Leveldb" + this->_status.ToString();
            //exit(-1);
        }
    }
    ~Level()
    {
        LOG(INFO) << "!!!!! LEVEL Destructor Called for db [" << _dbpath << "]";
        delete _db;
    }

  public:
    int Put(std::string key, std::string value)
    {
        _status = _db->Put(_writeoptions, key, value);
        if (!_status.ok())
            return -1;
        else
            return 0;
    }
    int Get(std::string key, std::string *value)
    {
        _status = _db->Get(_readoptions, key, value);
        if (!_status.ok())
            return -1;
        else
            return 0;
    }
    int Delete(std::string key)
    {
        _status = _db->Delete(_writeoptions, key);
        if (!_status.ok())
            return -1;
        else
            return 0;
    }

    void INCclientnum() { _clientsnum++; }
    void DECclientnum()
    {
        if (_clientsnum > 0)
            _clientsnum--;
    }
    int Getclientnum() { return _clientsnum; }

    std::string Getname() { return _dbpath; }

    std::string StatusStr()
    {
        return _status.ToString();
    }

  private:
    int _clientsnum;
    std::string _dbpath;
    leveldb::DB *_db;
    leveldb::Status _status;
    leveldb::Options _options;
    leveldb::WriteOptions _writeoptions;
    leveldb::ReadOptions _readoptions;
};

class Client
{
  public:
    Client(int32_t id, Level *db)
    {
        LOG(INFO) << "!!!!! Client Constructor Called for client [" << id << "]";
        _id = id;
        _db = db;
        _last_activate = time(NULL);
    }
    ~Client()
    {
        LOG(INFO) << "!!!!! Client Destructor Called for client [" << _id << "]";
    }
    uint32_t id() { return _id; }
    Level *db() { return _db; }
    void set_db(Level *db) { _db = db; }

  private:
    time_t _last_activate;
    uint32_t _id;
    Level *_db;
};

class Dlevel;

class HandlerClosure : public braft::Closure
{
  public:
    HandlerClosure(Dlevel *dlevel,
                   const DlevelRequest *request,
                   DlevelResponse *response,
                   google::protobuf::Closure *done)
        : _dlevel(dlevel), _request(request), _response(response), _done(done) {}

    ~HandlerClosure() {}

    const DlevelRequest *request() const { return _request; }
    DlevelResponse *response() const { return _response; }
    void Run();

  private:
    Dlevel *_dlevel;
    const DlevelRequest *_request;
    DlevelResponse *_response;
    google::protobuf::Closure *_done;
};

class Dlevel : public braft::StateMachine
{
  public:
    Dlevel()
        : _node(NULL), _leader_term(-1)
    {
    }
    ~Dlevel()
    {
        delete _node;
    }

  public:
    int start()
    {
        butil::EndPoint addr(butil::my_ip(), FLAGS_port);
        braft::NodeOptions node_options;
        if (node_options.initial_conf.parse_from(FLAGS_conf) != 0)
        {
            LOG(ERROR) << "Fail to parse configuration `" << FLAGS_conf << '\'';
            return -1;
        }
        node_options.election_timeout_ms = FLAGS_election_timeout_ms;
        node_options.fsm = this;
        node_options.node_owns_fsm = false;
        node_options.snapshot_interval_s = FLAGS_snapshot_interval;
        std::string prefix = "local://" + FLAGS_data_path;
        node_options.log_uri = prefix + "/log";
        node_options.raft_meta_uri = prefix + "/raft_meta";
        node_options.snapshot_uri = prefix + "/snapshot";
        node_options.disable_cli = FLAGS_disable_cli;
        braft::Node *node = new braft::Node(FLAGS_group, braft::PeerId(addr));
        LOG(INFO) << "\tNode is started: " << FLAGS_group << " " << braft::PeerId(addr);
        if (node->init(node_options) != 0)
        {
            LOG(ERROR) << "Fail to init raft node";
            delete node;
            return -1;
        }
        _node = node;
        return 0;
    }

    bool is_leader() const
    {
        return _leader_term.load(butil::memory_order_acquire) > 0;
    }

    void shutdown()
    {
        if (_node)
            _node->shutdown(NULL);
    }

    void join()
    {
        if (_node)
            _node->join();
    }

    void handler(const DlevelRequest *request,
                 DlevelResponse *response,
                 google::protobuf::Closure *done,
                 std::string addr)
    {
        LOG(INFO) << "Handle For client [addr:" << addr << " id:" << request->id() << " ]";
        brpc::ClosureGuard done_guard(done);
        const int64_t term = _leader_term.load(butil::memory_order_relaxed);
        if (term < 0)
            return _redirect(response);

        switch (request->actiontype())
        {
        case DlevelRequest_ActionType_GET:
            _get_handler(request, response);
            break;
        case DlevelRequest_ActionType_OPEN:
        case DlevelRequest_ActionType_CLOSE:
        case DlevelRequest_ActionType_DELETE:
        case DlevelRequest_ActionType_PUT:
            _open_close_put_delete_handler(request, response, done_guard.release());
            break;
        }
        return;
    }

  private:
    void _get_handler(const DlevelRequest *request,
                      DlevelResponse *response)
    {
        if (!is_leader())
            return _redirect(response);

        LOG(INFO) << "\t[_get_handle] called. for request key" << request->key();

        auto iterclient = _clients.find(request->id());
        if (iterclient == _clients.end())
        {
            if (response)
            {
                response->set_success(false);
                response->set_value("Please open a db first!");
            }
        }
        else
        {
            Client *client = iterclient->second;
            Level *db = client->db();
            std::string value;
            if ((db->Get(request->key(), &value)) != 0)
            {
                LOG(ERROR) << "\t\tcan't Get. " << db->StatusStr();
                if (response)
                {
                    response->set_success(false);
                    response->set_value(db->StatusStr());
                }
            }
            else
            {
                if (response)
                {
                    response->set_success(true);
                    response->set_value(value);
                }
            }
        }
    }

    void _open_close_put_delete_handler(const DlevelRequest *request,
                                        DlevelResponse *response,
                                        google::protobuf::Closure *done)
    {
        LOG(INFO) << "\t[_open_put_delete_handle] Called. Apply task to group";
        brpc::ClosureGuard done_guard(done);

        const int64_t term = _leader_term.load(butil::memory_order_relaxed);
        if (term < 0)
        {
            return _redirect(response);
        }

        butil::IOBuf log;
        butil::IOBufAsZeroCopyOutputStream wrapper(&log);
        if (!request->SerializeToZeroCopyStream(&wrapper))
        {
            LOG(ERROR) << "Fail to serialize request";
            response->set_success(false);
            return;
        }

        braft::Task task;
        task.data = &log;
        task.done = new HandlerClosure(this, request, response, done_guard.release());
        if (FLAGS_check_term)
            task.expected_term = term;

        _node->apply(task);
        return;
    }

    void on_apply(braft::Iterator &iter)
    {
        LOG(INFO) << "\t[on_apply] called.";
        // A batch of tasks are committed, which must be processed through
        // |iter|
        for (; iter.valid(); iter.next())
        {
            DlevelResponse *response = NULL;

            std::string key;
            std::string value;
            DlevelRequest_ActionType actiontype;
            int32_t id;

            // This guard helps invoke iter.done()->Run() asynchronously to
            // avoid that callback blocks the StateMachine.

            braft::AsyncClosureGuard closure_guard(iter.done());

            if (iter.done())
            {
                // This task is applied by this node, get value from this
                // closure to avoid additional parsing.
                HandlerClosure *c = dynamic_cast<HandlerClosure *>(iter.done());
                response = c->response();

                id = c->request()->id();
                key = c->request()->key();
                actiontype = c->request()->actiontype();
                if (c->request()->has_value())
                    value = c->request()->value();
            }
            else
            {
                // Have to parse DlevelRequest from this log.
                butil::IOBuf saved_log = iter.data();
                butil::IOBufAsZeroCopyInputStream wrapper(saved_log);
                DlevelRequest logrequest;
                CHECK(logrequest.ParseFromZeroCopyStream(&wrapper));

                id = logrequest.id();
                key = logrequest.key();
                actiontype = logrequest.actiontype();
                if (logrequest.has_value())
                    value = logrequest.value();
            }
            // Now the log has been parsed. Update this state machine by this
            // operation.
            switch (actiontype)
            {
            case DlevelRequest_ActionType_OPEN:
                _apply_open(id, key, response);
                break;
            case DlevelRequest_ActionType_PUT:
                _apply_put(id, key, value, response);
                break;
            case DlevelRequest_ActionType_DELETE:
                _apply_delete(id, key, response);
                break;
            case DlevelRequest_ActionType_CLOSE:
                _apply_close(id, response);
                break;
            // avoid warning...
            case DlevelRequest_ActionType_GET:
                break;
            }
            // The purpose of following logs is to help you understand the way
            // this StateMachine works.
            // Remove these logs in performance-sensitive servers.
            LOG(ERROR) << "\t\tapply finish";
        }
    }

  private:
    void _apply_open(int32_t id, std::string key, DlevelResponse *response)
    {
        LOG(INFO) << "\t\tDo Apply. ActionType: [ OPEN ].  Key:[" << key << "]";
        auto iterclient = _clients.find(id);
        if (iterclient == _clients.end())
        { // new client
            Client *client = new Client(id, NULL);
            auto iterlevel = _leveldbs.find(key);
            if (iterlevel == _leveldbs.end())
            { //
                LOG(INFO) << "\t\tOPEN a new LevelDB [" << key << "] for client [" << id << "]";
                //LOG(INFO) << "?HERE?";
                Level *db = new Level(key, 1);
                auto entrylevel = std::make_pair(key, db);
                //LOG(INFO) << "??HERE??";
                //auto checkpair =
                this->_leveldbs.insert(entrylevel);
                client->set_db(db);
                //LOG(INFO) << "????HERE????";
            }
            else
            {
                LOG(INFO) << "\t\tUse a OPENED LevelDB [" << key << "]";
                Level *db = iterlevel->second;
                db->INCclientnum();
                client->set_db(db);
            }
            //LOG(ERROR) << "????WHY????";
            auto entryclient = std::make_pair(id, client);
            //LOG(INFO) << "???CURSH???";
            this->_clients.insert(entryclient);
            if (response)
            {
                response->set_success(true);
                response->set_value("Success");
            }
            //LOG(INFO) << "???COME???";
        }
        else
        {
            if (response)
            {
                response->set_success(false);
                response->set_value("Already open!");
            }
        }
    }

    void _apply_put(int32_t id, std::string key, std::string value, DlevelResponse *response)
    {
        LOG(INFO) << "\t\tDo Apply. Id:[" << id << "]. ActionType: [ PUT ]. Key:[" << key << "]. Value:[" << value << "]";
        auto iterclient = _clients.find(id);
        if (iterclient == _clients.end())
        {
            if (response)
            {
                response->set_success(false);
                response->set_value("Please open a db first!");
            }
        }
        else
        {
            Client *client = iterclient->second;
            Level *db = client->db();
            if ((db->Put(key, value)) != 0)
            {
                LOG(ERROR) << "\t\tcan't apply. errstr [" << db->StatusStr() << "]";
                if (response)
                {
                    response->set_success(false);
                    response->set_value(db->StatusStr());
                }
            }
            else
            {
                LOG(INFO) << "\t\tapplied to db [" << db->Getname() << "]";
                if (response)
                {
                    response->set_success(true);
                    response->set_value("Success");
                }
            }
        }
    }

    void _apply_delete(int32_t id, std::string key, DlevelResponse *response)
    {
        LOG(INFO) << "\t\tDo Apply. Id:[" << id << "]. ActionType: [ DELETE ].  Key:[" << key << "]";
        auto iterclient = _clients.find(id);
        if (iterclient == _clients.end())
        {
            if (response)
            {
                response->set_success(false);
                response->set_value("Please open a db first!");
            }
        }
        else
        {
            Client *client = iterclient->second;
            Level *db = client->db();
            if ((db->Delete(key)) != 0)
            {
                LOG(ERROR) << "\t\tcan't apply " << db->StatusStr();
                if (response)
                {
                    response->set_success(false);
                    response->set_value(db->StatusStr());
                }
            }
            else
            {
                LOG(INFO) << "\t\tapplied to db [" << db->Getname() << "]";
                if (response)
                {
                    response->set_success(true);
                    response->set_value("Success");
                }
            }
        }
    }

    void _apply_close(int32_t id, DlevelResponse *response)
    {
        LOG(INFO) << "\t\tDo Apply. Id:[" << id << "]. ActionType: [ CLOSE ]";
        auto iterclient = _clients.find(id);
        if (iterclient == _clients.end())
        {
            if (response)
            {
                response->set_success(false);
                response->set_value("Not opened!");
            }
        }
        else
        {
            Client *client = iterclient->second;
            Level *db = client->db();
            this->_clients.erase(id);
            db->DECclientnum();
            if (db->Getclientnum() == 0)
            {
                LOG(INFO) << "LevelDB [" << db->Getname() << "] have no client. Close...";
                this->_leveldbs.erase(db->Getname());
                delete db;
            }
            delete client;
            if (response)
            {
                response->set_success(true);
                response->set_value("Success");
            }
        }
    }

    void _redirect(DlevelResponse *response)
    {
        response->set_success(false);
        if (_node)
        {
            braft::PeerId leader = _node->leader_id();
            if (!leader.is_empty())
                response->set_redirect(leader.to_string());
        }
    }

  private:
    void on_leader_start(int64_t term)
    {
        _leader_term.store(term, butil::memory_order_release);
        LOG(INFO) << "Node becomes leader";
    }
    void on_leader_stop(const butil::Status &status)
    {
        _leader_term.store(-1, butil::memory_order_release);
        LOG(INFO) << "Node stepped down : " << status;
    }
    void on_stop_following(const ::braft::LeaderChangeContext &ctx)
    {
        LOG(INFO) << "Node stops following " << ctx;
    }
    void on_start_following(const ::braft::LeaderChangeContext &ctx)
    {
        LOG(INFO) << "Node start following " << ctx;
    }
    void on_shutdown()
    {
        LOG(INFO) << "This node is down";
    }
    void on_error(const ::braft::Error &e)
    {
        LOG(ERROR) << "Met raft error " << e;
    }
    void on_configuration_committed(const ::braft::Configuration &conf)
    {
        LOG(INFO) << "Configuration of this group is " << conf;
    }
    void on_snapshot_save(braft::SnapshotWriter *writer, braft::Closure *done)
    {
        // Save current StateMachine in memory and starts a new bthread to avoid
        // blocking StateMachine since it's a bit slow to write data to disk
        // file.

        //!
    }

    int on_snapshot_load(braft::SnapshotReader *reader)
    {
        // Load snasphot from reader, replacing the running StateMachine
        //
        return 0;
    }

  private:
    braft::Node* volatile _node;
    butil::atomic<int64_t> _leader_term;
    std::map<int32_t, Client*> _clients;
    std::map<std::string, Level*> _leveldbs;

    friend class HandlerClosure;
};

/*
class Client
{
    Client()
}
*/

void HandlerClosure::Run()
{
    LOG(INFO) << "\tHandlerClosure.Run  called.";

    //std::unique_ptr<HandlerClosure> self_guard(this);
    brpc::ClosureGuard done_guard(_done);
    if (status().ok())
    {
        return;
    }
    _dlevel->_redirect(_response);
}

class DlevelServiceImpl : public DlevelService
{
  public:
    DlevelServiceImpl(Dlevel *dlevel) : _dlevel(dlevel) {}

    void handler(::google::protobuf::RpcController *controller,
                 const DlevelRequest *request,
                 DlevelResponse *response,
                 ::google::protobuf::Closure *done)
    {
        brpc::Controller *cntl = static_cast<brpc::Controller *>(controller);
        std::string addr = butil::endpoint2str(cntl->remote_side()).c_str();
        return _dlevel->handler(request, response, done, addr);
    }

  private:
    Dlevel *_dlevel;
};
} // namespace dlevel

using namespace dlevel;
int main(int argc, char *argv[])
{
    GFLAGS_NS::ParseCommandLineFlags(&argc, &argv, true);
    brpc::Server server;
    Dlevel dlevel;
    DlevelServiceImpl service(&dlevel);

    if (server.AddService(&service,
                          brpc::SERVER_DOESNT_OWN_SERVICE) != 0)
    {
        LOG(ERROR) << "Fail to add service";
        return -1;
    }

    if (braft::add_service(&server, FLAGS_port) != 0)
    {
        LOG(ERROR) << "Fail to add raft service";
        return -1;
    }

    if (server.Start(FLAGS_port, NULL) != 0)
    {
        LOG(ERROR) << "Fail to start Server";
        return -1;
    }

    if (dlevel.start() != 0)
    {
        LOG(ERROR) << "Fail to start dlevel";
        return -1;
    }

    LOG(INFO) << "Dlevel service is running on " << server.listen_address();

    while (!brpc::IsAskedToQuit())
    {
        sleep(1);
    }

    LOG(INFO) << "Counter service is going to quit";

    dlevel.shutdown();
    server.Stop(0);

    dlevel.join();
    server.Join();
    return 0;
}