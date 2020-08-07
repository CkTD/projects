package kvraft

import (
	"bytes"
	"fmt"
	"sync"
	"sync/atomic"
	"time"

	"../labgob"
	"../labrpc"
	"../raft"
)

const Debug = 1

type Op struct {
	// Your definitions here.
	// Field names must start with capital letters,
	// otherwise RPC will break.
	Op    string
	Key   string
	Value string

	ClientID  int64
	MessageID int64
}

type OpIdentifier struct {
	Term  int
	Index int
}

type OpResult struct {
	Err   Err
	Value string
}

type Latest struct {
	MessageID int64
	Result    OpResult
}

type KVServer struct {
	mu      sync.Mutex
	me      int
	rf      *raft.Raft
	applyCh chan raft.ApplyMsg
	dead    int32 // set by Kill()

	maxraftstate int // snapshot if log grows this big

	// Your definitions here.
	lastIndex int

	table         map[string]string              // key -> value
	opCh          map[OpIdentifier]chan OpResult // Index -> channel waiting for result
	latestMessage map[int64]Latest               // clientID -> {latestMessageID, OpResult}
}

func (kv *KVServer) DPrintf(format string, a ...interface{}) (n int, err error) {
	if Debug > 0 {
		msg := fmt.Sprintf("%v kv[%d] ", time.Now().Format("15:04:05.000000"), kv.me) + format + "\n"
		fmt.Printf(msg, a...)
	}
	return
}

func wait(ch chan OpResult) (OpResult, bool) {
	select {
	case res := <-ch:
		return res, true
	case <-time.After(time.Second):
		return OpResult{}, false
	}
}

func (kv *KVServer) Get(args *GetArgs, reply *GetReply) {
	// Your code here.

	cmd := Op{Op: "Get",
		Key:       args.Key,
		ClientID:  args.ClientID,
		MessageID: args.MessageID}

	index, term, isLeader := kv.rf.Start(cmd)

	if !isLeader {
		reply.Err = ErrWrongLeader
	} else {
		kv.DPrintf("start get for[%d %d] %s...",
			args.ClientID, args.MessageID, args.Key)
		ch := make(chan OpResult)
		opI := OpIdentifier{Index: index, Term: term}

		kv.mu.Lock()
		kv.opCh[opI] = ch
		kv.mu.Unlock()

		res, ok := wait(ch)

		if ok {
			reply.Err = OK
			reply.Value = res.Value
			kv.DPrintf("done get for[%d %d] %s->%s",
				args.ClientID, args.MessageID, args.Key, res.Value)
		} else {
			reply.Err = ErrWrongLeader
		}
		kv.mu.Lock()
		delete(kv.opCh, opI)
		kv.mu.Unlock()
	}
}

func (kv *KVServer) PutAppend(args *PutAppendArgs, reply *PutAppendReply) {
	// Your code here.

	cmd := Op{Op: args.Op,
		Key:       args.Key,
		Value:     args.Value,
		ClientID:  args.ClientID,
		MessageID: args.MessageID}

	index, term, isLeader := kv.rf.Start(cmd)

	if !isLeader {
		reply.Err = ErrWrongLeader
	} else {
		kv.DPrintf("start %s for [%d %d] %s[%s]",
			args.Op, args.ClientID, args.MessageID, args.Key, args.Value)
		ch := make(chan OpResult)
		opI := OpIdentifier{Index: index, Term: term}

		kv.mu.Lock()
		kv.opCh[opI] = ch
		kv.mu.Unlock()

		res, ok := wait(ch)

		if ok {
			reply.Err = res.Err
			reply.Value = res.Value
			kv.DPrintf("done %s for [%d %d] %s->%s",
				args.Op, args.ClientID, args.MessageID, args.Key, res.Value)
		} else {
			reply.Err = ErrWrongLeader
		}
		kv.mu.Lock()
		delete(kv.opCh, opI)
		kv.mu.Unlock()
	}
}

//
// the tester calls Kill() when a KVServer instance won't
// be needed again. for your convenience, we supply
// code to set rf.dead (without needing a lock),
// and a killed() method to test rf.dead in
// long-running loops. you can also add your own
// code to Kill(). you're not required to do anything
// about this, but it may be convenient (for example)
// to suppress debug output from a Kill()ed instance.
//
func (kv *KVServer) Kill() {
	atomic.StoreInt32(&kv.dead, 1)
	kv.rf.Kill()
	// Your code here, if desired.
	kv.DPrintf("stopped")
}

func (kv *KVServer) killed() bool {
	z := atomic.LoadInt32(&kv.dead)
	return z == 1
}

func (kv *KVServer) apply() {
	for m := range kv.applyCh {
		if !m.CommandValid {
			kv.readSnapshot(m.Command.([]byte))
			continue
		}

		op := m.Command.(Op)
		index := m.CommandIndex
		term := m.CommandTerm
		clientID := op.ClientID
		messageID := op.MessageID

		var res OpResult

		kv.mu.Lock()
		latest, exist := kv.latestMessage[clientID]
		if exist && messageID == latest.MessageID {
			res = latest.Result
			kv.DPrintf("duplicate req for log[%d] op:[%v] cached res:[%v]", index, op, res)
		} else {
			switch op.Op {
			case "Get":
				if val, ok := kv.table[op.Key]; ok {
					res = OpResult{Err: OK, Value: val}
				} else {
					res = OpResult{Err: ErrNoKey}
				}
			case "Put":
				kv.table[op.Key] = op.Value
				res = OpResult{Err: OK, Value: kv.table[op.Key]}
			case "Append":
				kv.table[op.Key] += op.Value
				res = OpResult{Err: OK, Value: kv.table[op.Key]}
			}
			kv.DPrintf("apply log[%d] op:%v res:%v", index, op, res)
			kv.latestMessage[clientID] = Latest{MessageID: messageID, Result: res}
		}

		ch, exist := kv.opCh[OpIdentifier{Term: term, Index: index}]
		if exist {
			kv.DPrintf("reply to [%d] log[%d]:%v", clientID, index, op)
			// The msg in the channel must be read as soon as possible. deadlock?
			ch <- res
		}
		kv.lastIndex = index
		kv.mu.Unlock()
	}
}

// monitor the raft state size, snapshot if it is too big
func (kv *KVServer) snapshot() {
	for s := range kv.rf.SnapshotCh {
		if s.Force || s.StateSize >= kv.maxraftstate {
			kv.DPrintf("snapshot: current state size: [%d], max: [%d]", s, kv.maxraftstate)

			w := new(bytes.Buffer)
			e := labgob.NewEncoder(w)

			kv.mu.Lock()
			lastIndex := kv.lastIndex
			e.Encode(kv.lastIndex)
			e.Encode(kv.table)
			e.Encode(kv.latestMessage)
			kv.mu.Unlock()

			data := w.Bytes()
			kv.rf.SnapShot(data, lastIndex)
		}

	}
}

func (kv *KVServer) readSnapshot(snapshot []byte) {
	if snapshot == nil || len(snapshot) < 1 { // bootstrap without any state?
		return
	}
	kv.mu.Lock()
	r := bytes.NewBuffer(snapshot)
	d := labgob.NewDecoder(r)
	var table map[string]string
	var latestMessage map[int64]Latest
	var lastIndex int
	if d.Decode(&lastIndex) != nil ||
		d.Decode(&table) != nil ||
		d.Decode(&latestMessage) != nil {
		panic("read snapshot error")
	} else {
		kv.DPrintf("load snapshot[%d]", lastIndex)
		kv.lastIndex = lastIndex
		kv.table = table
		kv.latestMessage = latestMessage
	}
	kv.mu.Unlock()
}

//
// servers[] contains the ports of the set of
// servers that will cooperate via Raft to
// form the fault-tolerant key/value service.
// me is the index of the current server in servers[].
// the k/v server should store snapshots through the underlying Raft
// implementation, which should call persister.SaveStateAndSnapshot() to
// atomically save the Raft state along with the snapshot.
// the k/v server should snapshot when Raft's saved state exceeds maxraftstate bytes,
// in order to allow Raft to garbage-collect its log. if maxraftstate is -1,
// you don't need to snapshot.
// StartKVServer() must return quickly, so it should start goroutines
// for any long-running work.
//
func StartKVServer(servers []*labrpc.ClientEnd, me int, persister *raft.Persister, maxraftstate int) *KVServer {
	// call labgob.Register on structures you want
	// Go's RPC library to marshall/unmarshall.
	labgob.Register(Op{})

	kv := new(KVServer)
	kv.me = me
	kv.maxraftstate = maxraftstate

	// You may need initialization code here.

	kv.applyCh = make(chan raft.ApplyMsg)
	kv.rf = raft.Make(servers, me, persister, kv.applyCh)

	// You may need initialization code here.
	kv.lastIndex = 0
	kv.table = make(map[string]string)
	kv.opCh = make(map[OpIdentifier]chan OpResult)
	kv.latestMessage = make(map[int64]Latest)

	kv.readSnapshot(persister.ReadSnapshot())

	go kv.apply()
	if kv.maxraftstate != -1 {
		go kv.snapshot()
	}

	kv.DPrintf("started")

	return kv
}
