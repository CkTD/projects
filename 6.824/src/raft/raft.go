package raft

//
// this is an outline of the API that raft must expose to
// the service (or tester). see comments below for
// each of these functions for more details.
//
// rf = Make(...)
//   create a new Raft server.
// rf.Start(command interface{}) (index, term, isleader)
//   start agreement on a new log entry
// rf.GetState() (term, isLeader)
//   ask a Raft for its current term, and whether it thinks it is leader
// ApplyMsg
//   each time a new entry is committed to the log, each Raft peer
//   should send an ApplyMsg to the service (or tester)
//   in the same server.
//

import (
	"bytes"
	"fmt"
	"sync"
	"sync/atomic"
	"time"

	"../labgob"
	"../labrpc"
)

// import "bytes"
// import "../labgob"

//
// as each Raft peer becomes aware that successive log entries are
// committed, the peer should send an ApplyMsg to the service (or
// tester) on the same server, via the applyCh passed to Make(). set
// CommandValid to true to indicate that the ApplyMsg contains a newly
// committed log entry.
//
// in Lab 3 you'll want to send other kinds of messages (e.g.,
// snapshots) on the applyCh; at that point you can add fields to
// ApplyMsg, but set CommandValid to false for these other uses.
//
type ApplyMsg struct {
	CommandValid bool
	Command      interface{}
	CommandIndex int
	CommandTerm  int
}

// after Raft log size change or Raft want to make a latest snapsht
// raft send an SnapShotMsg to the service
type SnapShotMsg struct {
	StateSize int
	Force     bool
}

const (
	candidate = 0
	leader    = 1
	follower  = 2
)

//
// A Go object implementing a single Raft peer.
//
type Raft struct {
	mu        sync.Mutex          // Lock to protect shared access to this peer's state
	peers     []*labrpc.ClientEnd // RPC end points of all peers
	persister *Persister          // Object to hold this peer's persisted state
	me        int                 // this peer's index into peers[]
	dead      int32               // set by Kill()
	applyCH   chan ApplyMsg

	// Your data here (2A, 2B, 2C).
	// Look at the paper's Figure 2 for a description of what
	// state a Raft server must maintain.

	//Persistent state
	currentTerm int
	votedFor    int
	log         *Log

	//Volatile state on all servers
	commitIndex int
	lastApplied int

	//Volatile state on leaders
	nextIndex  []int
	matchIndex []int

	//Other Volatile State
	state              int
	leaderID           int
	gatheredVots       int
	lastElectionActive time.Time
	lastAppendEntries  time.Time

	//Some constants
	peersNum              int
	majorityNum           int
	requestVoteTimeoutMin time.Duration
	requestVoteTimeoutMax time.Duration
	appendEntriesInterval time.Duration

	//For snapshot
	SnapshotCh chan SnapShotMsg
}

// Debugging
const Debug = 1

// DPrintf print msg with some important state
func (rf *Raft) DPrintf(format string, a ...interface{}) (n int, err error) {
	state := ""
	switch rf.state {
	case follower:
		state = "follower"
	case candidate:
		state = "candidate"
	case leader:
		state = "leader"
	}
	msg := fmt.Sprintf("%v raft[%d] term[%d] state[%-9s] ", time.Now().Format("15:04:05.000000"), rf.me, rf.currentTerm, state)
	msg = msg + format + "\n"
	if Debug > 0 {
		fmt.Printf(msg, a...)
	}
	return
}

// encode Raft's persistence state
// caller lock the mutex
func (rf *Raft) encodeState() []byte {
	w := new(bytes.Buffer)
	e := labgob.NewEncoder(w)

	e.Encode(rf.currentTerm)
	e.Encode(rf.votedFor)
	e.Encode(rf.log)

	state := w.Bytes()
	return state
}

// save Raft's persistent state to stable storage,
// and notify state size change
// caller lock the mutex
func (rf *Raft) persist() {
	// Your code here (2C).

	state := rf.encodeState()
	rf.persister.SaveRaftState(state)

	select {
	case rf.SnapshotCh <- SnapShotMsg{rf.persister.RaftStateSize(), false}:
	default:
	}

}

//
// restore previously persisted state.
// caller lock the mutex
func (rf *Raft) readPersist(state []byte) {
	if state == nil || len(state) < 1 { // bootstrap without any state?
		return
	}
	// Your code here (2C).
	r := bytes.NewBuffer(state)
	d := labgob.NewDecoder(r)
	var currentTerm int
	var voteFor int
	var log *Log
	if d.Decode(&currentTerm) != nil ||
		d.Decode(&voteFor) != nil ||
		d.Decode(&log) != nil {
		panic("read persist error")
	} else {
		rf.DPrintf("load persist state, Term:[%d], voteFor:[%d], last log index: [%d]", currentTerm, voteFor, log.LastIndex())
		rf.currentTerm = currentTerm
		rf.votedFor = voteFor
		rf.log = log
		rf.matchIndex[rf.me] = rf.log.LastIndex()
		rf.commitIndex = rf.log.PrevIndex
		rf.lastApplied = rf.log.PrevIndex
	}
}

// change state to follower
func (rf *Raft) toFollower(term, leaderID int) {
	rf.state = follower
	rf.lastElectionActive = time.Now()

	if term > rf.currentTerm {
		rf.DPrintf("new term [%d] detected", term)
		rf.currentTerm = term
		rf.votedFor = -1
		rf.leaderID = -1
	}
	if leaderID != rf.leaderID {
		rf.DPrintf("new leader [%d] detected", leaderID)
		rf.leaderID = leaderID
	}
}

// RequestVote RPC arguments structure.
// field names must start with capital letters!
type RequestVoteArgs struct {
	// Your data here (2A, 2B).
	Term         int
	CandidateID  int
	LastLogIndex int
	LastLogTerm  int
}

// RequestVote RPC reply structure.
// field names must start with capital letters!
type RequestVoteReply struct {
	// Your data here (2A).
	Term        int
	VoteGranted bool
}

// RequestVote RPC handler.
func (rf *Raft) RequestVote(args *RequestVoteArgs, reply *RequestVoteReply) {
	// Your code here (2A, 2B).
	rf.mu.Lock()
	defer rf.mu.Unlock()

	reply.Term = rf.currentTerm
	reply.VoteGranted = false

	// follow up if necessary
	if args.Term > rf.currentTerm {
		rf.toFollower(args.Term, -1)
	}

	// vote if possible
	if args.Term == rf.currentTerm && // The candidate must have latest term
		(rf.votedFor == -1 || rf.votedFor == args.CandidateID) && // We havn't vote any other peer in this term
		(args.LastLogTerm > rf.log.LastTerm() || // The candidate's log is as up-to-date as ours
			(args.LastLogTerm == rf.log.LastTerm() && args.LastLogIndex >= rf.log.LastIndex())) {
		rf.votedFor = args.CandidateID
		reply.Term = rf.currentTerm
		reply.VoteGranted = true
		rf.lastElectionActive = time.Now()
		rf.DPrintf("vote for [%d]", args.CandidateID)
	}

	rf.persist()
}

//
// example code to send a RequestVote RPC to a server.
// server is the index of the target server in rf.peers[].
// expects RPC arguments in args.
// fills in *reply with RPC reply, so caller should
// pass &reply.
// the types of the args and reply passed to Call() must be
// the same as the types of the arguments declared in the
// handler function (including whether they are pointers).
//
// The labrpc package simulates a lossy network, in which servers
// may be unreachable, and in which requests and replies may be lost.
// Call() sends a request and waits for a reply. If a reply arrives
// within a timeout interval, Call() returns true; otherwise
// Call() returns false. Thus Call() may not return for a while.
// A false return can be caused by a dead server, a live server that
// can't be reached, a lost request, or a lost reply.
//
// Call() is guaranteed to return (perhaps after a delay) *except* if the
// handler function on the server side does not return.  Thus there
// is no need to implement your own timeouts around Call().
//
// look at the comments in ../labrpc/labrpc.go for more details.
//
// if you're having trouble getting RPC to work, check that you've
// capitalized all field names in structs passed over RPC, and
// that the caller passes the address of the reply struct with &, not
// the struct itself.
//
func (rf *Raft) sendRequestVote(server int, args *RequestVoteArgs, reply *RequestVoteReply) bool {
	ok := rf.peers[server].Call("Raft.RequestVote", args, reply)
	return ok
}

type AppendEntriesArgs struct {
	Term         int
	LeaderID     int
	PrevLogIndex int
	PrevLogTerm  int
	Entries      []LogEntry
	LeaderCommit int
}

type AppendEntriesReply struct {
	Term    int
	Success bool

	// If follower reject AE, following information help leader
	// figure out conflict log
	XTerm  int // Term of conflicting term
	XIndex int // Index of first entry in XTerm
	XLen   int // Length of logs
}

func (rf *Raft) AppendEntries(args *AppendEntriesArgs, reply *AppendEntriesReply) {
	rf.mu.Lock()
	defer rf.mu.Unlock()

	// leader out of date
	if args.Term < rf.currentTerm {
		reply.Success = false
		reply.Term = rf.currentTerm
		return
	}

	// follow up if necessary
	rf.toFollower(args.Term, args.LeaderID)
	reply.Term = args.Term

	// consistency check
	reply.Success = false
	s := rf.log.Find(args.PrevLogTerm, args.PrevLogIndex)

	// consistency check: failed
	// expected log has been discarded, let leader send from begining
	if s == discarded {
		reply.XLen = 0
		reply.XTerm = 0
		reply.XIndex = 0
		return
	}

	// consistency check: conflict
	if s == notexist {
		if rf.log.LastIndex() >= args.PrevLogIndex {
			reply.XLen = args.PrevLogIndex
			reply.XTerm = rf.log.GetTerm(args.PrevLogIndex)
			reply.XIndex = args.PrevLogIndex
		} else {
			reply.XLen = rf.log.LastIndex()
			reply.XTerm = rf.log.LastTerm()
			reply.XIndex = rf.log.LastIndex()
		}
		for reply.XIndex > rf.log.PrevIndex &&
			rf.log.GetTerm(reply.XIndex-1) == reply.XTerm {
			reply.XIndex--
		}
		return
	}

	// consistency check: pass
	reply.Success = true
	if len(args.Entries) != 0 {
		AELIndex := args.PrevLogIndex + len(args.Entries)
		AELTerm := args.Entries[len(args.Entries)-1].Term

		// ignores log entries already exist, rpc should be idempotent
		if rf.log.Find(AELTerm, AELIndex) != exist {
			// overwrite conflict logs
			if rf.log.LastIndex() > args.PrevLogIndex {
				rf.DPrintf("AE: overwrite conflict logs after[%d], last is: %d ",
					args.PrevLogIndex, rf.log.LastIndex())
			}
			rf.log.DeleteAfter(args.PrevLogIndex)

			// append logs
			rf.log.Append(args.Entries...)
			rf.DPrintf("AE: [%d] logs received, new last is [%d]",
				len(args.Entries), rf.log.LastIndex())
		}
	}

	// persist
	rf.persist()

	// commit and apply if any
	if args.LeaderCommit > rf.commitIndex {
		rf.DPrintf("commit log [%d]-[%d]", rf.commitIndex+1, args.LeaderCommit)
		rf.commitIndex = Min(args.LeaderCommit, rf.log.LastIndex())
	}
}

func (rf *Raft) sendAppendEntries(server int, args *AppendEntriesArgs, reply *AppendEntriesReply) bool {
	ok := rf.peers[server].Call("Raft.AppendEntries", args, reply)
	return ok
}

type InstallSnapshotArgs struct {
	Term              int
	LeaderID          int
	LastIncludedTerm  int
	LastIncludedIndex int
	Data              []byte
}

type InstallSnapshotReply struct {
	Term int
}

func (rf *Raft) InstallSnapshot(args *InstallSnapshotArgs, reply *InstallSnapshotReply) {
	rf.mu.Lock()
	defer rf.mu.Unlock()

	// leader out of date
	if args.Term < rf.currentTerm {
		reply.Term = rf.currentTerm
		return
	}

	// follow up if necessary
	rf.toFollower(args.Term, args.LeaderID)
	reply.Term = args.Term

	// ignore outdated install snapshot rpc
	if rf.log.Find(args.LastIncludedTerm, args.LastIncludedIndex) == exist {
		rf.DPrintf("install snapshot: ignore, already exist. LastIncluded[Term %d Index %d], current last[%d, %d]",
			args.LastIncludedTerm, args.LastIncludedIndex, rf.log.LastTerm(), rf.log.LastIndex())
		return
	}

	if rf.log.LastTerm() == args.Term && rf.log.LastIndex() > args.LastIncludedIndex {
		rf.DPrintf("install snapshot: ignore, don't overwrite. LastIncluded[Term %d Index %d], current last[%d, %d]",
			args.LastIncludedTerm, args.LastIncludedIndex, rf.log.LastTerm(), rf.log.LastIndex())
		return
	}

	// discard log
	rf.DPrintf("install Snapshot: LastIncluded[Term %d Index %d], current last[%d, %d]",
		args.LastIncludedTerm, args.LastIncludedIndex, rf.log.LastTerm(), rf.log.LastIndex())
	rf.log = MakeLog(args.LastIncludedTerm, args.LastIncludedIndex)
	rf.lastApplied = args.LastIncludedIndex
	rf.commitIndex = args.LastIncludedIndex

	state := rf.encodeState()
	rf.persister.SaveStateAndSnapshot(state, args.Data)

	// install the snapshot to application
	msg := ApplyMsg{
		CommandValid: false,
		Command:      args.Data,
		CommandIndex: 0,
		CommandTerm:  0,
	}

	rf.applyCH <- msg
}

func (rf *Raft) sendInstallSnapshot(server int, args *InstallSnapshotArgs, reply *InstallSnapshotReply) bool {
	ok := rf.peers[server].Call("Raft.InstallSnapshot", args, reply)
	return ok
}

func (rf *Raft) requestVoteToServer(s, term int) {
	rf.mu.Lock()
	defer rf.mu.Unlock()

	// after the lock is acquired, the state may have changed, check it
	if rf.state != candidate || rf.currentTerm != term {
		return
	}

	args := RequestVoteArgs{
		Term:         rf.currentTerm,
		CandidateID:  rf.me,
		LastLogTerm:  rf.log.LastTerm(),
		LastLogIndex: rf.log.LastIndex()}
	reply := RequestVoteReply{}

	rf.mu.Unlock()
	ok := rf.sendRequestVote(s, &args, &reply)
	rf.mu.Lock()

	if !ok || args.Term != rf.currentTerm { // ignore failed RPC & reply from old term
	} else if reply.Term > rf.currentTerm { // we are left behind, follow up
		rf.toFollower(reply.Term, -1)
	} else if rf.state == candidate && // handle a valid vote -> try to be leader.
		reply.VoteGranted == true {
		rf.gatheredVots++
		if rf.gatheredVots >= rf.majorityNum {
			rf.state = leader
			for i := range rf.nextIndex {
				rf.nextIndex[i] = rf.log.LastIndex() + 1
				rf.matchIndex[i] = 0
			}
			rf.matchIndex[rf.me] = rf.log.LastIndex()
			rf.DPrintf("become leader, lastLogIndex:%d", rf.log.LastIndex())
		}
	}
}

func (rf *Raft) appendEntryToServer(s, term int) {
	rf.mu.Lock()
	defer rf.mu.Unlock()

	// after the lock is acquired, the state may have changed, checke it
	if rf.state != leader || rf.currentTerm != term {
		return
	}

	l, ok := rf.log.GetRange(rf.nextIndex[s], rf.log.LastIndex())

	// previous snapshot has discarded logs needed by flollower
	if !ok {
		go rf.installSnapshotToServer(s, term)
		return
	}

	args := AppendEntriesArgs{
		Term:         rf.currentTerm,
		LeaderID:     rf.me,
		PrevLogIndex: rf.nextIndex[s] - 1,
		PrevLogTerm:  rf.log.GetTerm(rf.nextIndex[s] - 1),
		Entries:      l,
		LeaderCommit: rf.commitIndex,
	}
	reply := AppendEntriesReply{}

	rf.mu.Unlock()
	ok = rf.sendAppendEntries(s, &args, &reply)
	rf.mu.Lock()

	if !ok || args.Term != rf.currentTerm { // ignore failed RPC & reply from old term
	} else if reply.Term > rf.currentTerm { // we are left behind -> follow up
		rf.toFollower(reply.Term, -1)
	} else if rf.matchIndex[s] >= args.PrevLogIndex+len(l) { // ignore delayed reply in this term
		if len(l) > 0 {
			rf.DPrintf("Ignore outdated AE reply from [%d] that copied log[%d], current matchIndex[%d]",
				s, args.PrevLogIndex+len(args.Entries), rf.matchIndex[s])
		}
	} else if !reply.Success { // AE conflict -> decrease nextIndex
		rf.DPrintf("AE: log[%d] failed for follower [%d] with XIndex:%d XTerm:%d",
			args.PrevLogIndex+1, s, reply.XIndex, reply.XTerm)
		nextIndex := reply.XIndex
		for nextIndex >= rf.log.PrevIndex &&
			nextIndex <= reply.XLen &&
			rf.log.GetTerm(nextIndex) == reply.XTerm {
			nextIndex++
		}
		rf.nextIndex[s] = nextIndex
		Assert(rf.nextIndex[s] <= rf.log.LastIndex()+1, "index out of range")
	} else if len(l) > 0 { // AE success and some logs copied to follower -> try commit
		rf.DPrintf("AE success for follower[%d], matchIndex[%d]->[%d]",
			s, rf.matchIndex[s], args.PrevLogIndex+len(l))
		rf.nextIndex[s] = args.PrevLogIndex + len(l) + 1
		rf.matchIndex[s] = rf.nextIndex[s] - 1

		Assert(rf.nextIndex[s] <= rf.log.LastIndex()+1, "index out of range")

		// Check commit
		N := rf.log.LastIndex()
		for N > rf.commitIndex {
			if rf.log.GetTerm(N) == rf.currentTerm {
				a := 0
				for _, m := range rf.matchIndex {
					if m >= N {
						a++
					}
				}
				if a >= rf.majorityNum {
					break
				}
			}
			N--
		}
		if N != rf.commitIndex {
			rf.DPrintf("commit index [%d]->[%d]", rf.commitIndex, N)
			rf.commitIndex = N
		}
	}
}

func (rf *Raft) installSnapshotToServer(s, term int) {
	rf.mu.Lock()
	defer rf.mu.Unlock()

	// after the lock is acquired, the state may have changed, check it
	if rf.state != leader || rf.currentTerm != term {
		return
	}

	// never delete logs that have been successfully replicated to follower
	// nofity service to update snapshot and try again later
	if rf.matchIndex[s] > rf.log.PrevIndex ||
		(rf.log.LastTerm() < rf.currentTerm && rf.log.PrevIndex != rf.log.LastIndex()) {
		select {
		case rf.SnapshotCh <- SnapShotMsg{rf.persister.RaftStateSize(), true}:
		default:
		}
		return
	}

	rf.DPrintf("install snapshot to [%d] for index: [%d], nextIndex: [%d]", s, rf.log.PrevIndex, rf.nextIndex[s])
	args := InstallSnapshotArgs{Term: rf.currentTerm,
		LeaderID:          rf.me,
		LastIncludedTerm:  rf.log.PrevTerm,
		LastIncludedIndex: rf.log.PrevIndex,
		Data:              rf.persister.ReadSnapshot()}
	reply := InstallSnapshotReply{}

	rf.mu.Unlock()
	ok := rf.sendInstallSnapshot(s, &args, &reply)
	rf.mu.Lock()

	if !ok || args.Term != rf.currentTerm { // ignore failed RPC & reply from old term
	} else if reply.Term > rf.currentTerm { // we are left behind, follow up
		rf.toFollower(reply.Term, -1)
	} else {
		rf.nextIndex[s] = Max(args.LastIncludedIndex+1, rf.nextIndex[s])
		rf.matchIndex[s] = Max(args.LastIncludedIndex, rf.matchIndex[s])
		rf.DPrintf("install snapshot to [%d] success, change nextIndex to [%d]", s, rf.nextIndex[s])
	}
}

// caller Lock the mutex
// return immediately
func (rf *Raft) electionTimeoutHandler() {
	rf.lastElectionActive = time.Now()

	rf.currentTerm++
	term := rf.currentTerm
	rf.state = candidate
	rf.votedFor = rf.me
	rf.gatheredVots = 1
	for s := range rf.peers {
		if s == rf.me {
			continue
		}
		go rf.requestVoteToServer(s, term)
	}
}

// caller Lock the mutex
// return immediately
func (rf *Raft) appendEntryTimeoutHandler() {
	rf.lastAppendEntries = time.Now()
	for s := range rf.peers {
		if s == rf.me {
			continue
		}
		go rf.appendEntryToServer(s, rf.currentTerm)
	}
}

// caller lock the mutex
// return immediately
func (rf *Raft) applyLogHandler() {
	for rf.commitIndex > rf.lastApplied {
		rf.lastApplied++
		msg := ApplyMsg{
			CommandValid: true,
			Command:      rf.log.GetCommand(rf.lastApplied),
			CommandIndex: rf.lastApplied,
			CommandTerm:  rf.log.GetTerm(rf.lastApplied),
		}
		rf.DPrintf("apply log[%d]", msg.CommandIndex)
		rf.applyCH <- msg
	}
}

func (rf *Raft) periodicTask() {
	for {
		time.Sleep(time.Millisecond * 20)

		rf.mu.Lock()
		if rf.state == leader { // leader send appendEntries periodically
			if time.Since(rf.lastAppendEntries) > rf.appendEntriesInterval {
				rf.DPrintf("send append entries")
				rf.appendEntryTimeoutHandler()
			}
		} else { // candidate and follower check election timeout periodically
			timeout := RandIntRange(rf.requestVoteTimeoutMin, rf.requestVoteTimeoutMax)
			if time.Since(rf.lastElectionActive) > timeout {
				rf.DPrintf("election timeout")
				rf.electionTimeoutHandler()
			}
		}
		// apply log periodically here is much easier than call it every time new logs are
		// commited (log must be applied in order and avoid deadlock when snapshot)
		rf.applyLogHandler()
		rf.mu.Unlock()
	}
}

// return currentTerm and whether this server
// believes it is the leader.
func (rf *Raft) GetState() (int, bool) {

	var term int
	var isleader bool

	// Your code here (2A).
	rf.mu.Lock()
	term = rf.currentTerm
	isleader = rf.state == leader
	rf.mu.Unlock()

	return term, isleader
}

// the tester doesn't halt goroutines created by Raft after each test,
// but it does call the Kill() method. your code can use killed() to
// check whether Kill() has been called. the use of atomic avoids the
// need for a lock.
//
// the issue is that long-running goroutines use memory and may chew
// up CPU time, perhaps causing later tests to fail and generating
// confusing debug output. any goroutine with a long-running loop
// should call killed() to check whether it should stop.
//
func (rf *Raft) Kill() {
	atomic.StoreInt32(&rf.dead, 1)
	// Your code here, if desired.
	rf.DPrintf("stopped")
}

func (rf *Raft) killed() bool {
	z := atomic.LoadInt32(&rf.dead)
	return z == 1
}

//
// the service using Raft (e.g. a k/v server) wants to start
// agreement on the next command to be appended to Raft's log. if this
// server isn't the leader, returns false. otherwise start the
// agreement and return immediately. there is no guarantee that this
// command will ever be committed to the Raft log, since the leader
// may fail or lose an election. even if the Raft instance has been killed,
// this function should return gracefully.
//
// the first return value is the index that the command will appear at
// if it's ever committed. the second return value is the current
// term. the third return value is true if this server believes it is
// the leader.
//
func (rf *Raft) Start(command interface{}) (int, int, bool) {
	index := -1
	term := -1
	isLeader := true
	// Your code here (2B).
	rf.mu.Lock()
	isLeader = rf.state == leader
	if isLeader {
		rf.log.Append(LogEntry{Term: rf.currentTerm, Command: command})
		rf.matchIndex[rf.me] = rf.log.LastIndex()
		index = rf.log.LastIndex()
		term = rf.currentTerm
		rf.persist() // for leader, persist is necessary for new log
		rf.DPrintf("start a new command, index[%d]", rf.log.LastIndex())
		// pass test quickly...
		rf.appendEntryTimeoutHandler()
	}
	rf.mu.Unlock()

	return index, term, isLeader
}

//
// the service may want raft to shrink the log size when it is too big
// it take a snapshot corresponding to a log index, Raft can safely
// discared all the log before that index and save the snapshot along
// with current state
func (rf *Raft) SnapShot(snapshot []byte, lastIncludedIndex int) {
	rf.mu.Lock()
	defer rf.mu.Unlock()

	// afther get the lock, state may already changed
	// ignore when leader already install an snapshot after service took
	// this snapshot:
	// 1. a leader may install an snapshot that includeing less logs than this
	//	snapshot, but we can't overrite snapshot given by leader(leader need
	//	that to do consistency check) or
	// 2. leader installed snapshot that includeing more logs than this
	// ignore when another succeeded SnapShot including more logs than this
	if rf.lastApplied < lastIncludedIndex || rf.log.PrevIndex >= lastIncludedIndex {
		rf.DPrintf("take snapshot: ignore[%d], installed an snapshot from leader before we can apply the one given by service", lastIncludedIndex)
		return
	}

	rf.DPrintf("take snapshot: for index: [%d], last index: [%d]", lastIncludedIndex, rf.log.LastIndex())

	rf.log.DiscardBefore(lastIncludedIndex + 1)
	state := rf.encodeState()
	oldSize := rf.persister.RaftStateSize()
	rf.persister.SaveStateAndSnapshot(state, snapshot)
	newSize := rf.persister.RaftStateSize()

	rf.DPrintf("take snapshot done: state [%d]->[%d] bytes", oldSize, newSize)
}

//
// the service or tester wants to create a Raft server. the ports
// of all the Raft servers (including this one) are in peers[]. this
// server's port is peers[me]. all the servers' peers[] arrays
// have the same order. persister is a place for this server to
// save its persistent state, and also initially holds the most
// recent saved state, if any. applyCh is a channel on which the
// tester or service expects Raft to send ApplyMsg messages.
// Make() must return quickly, so it should start goroutines
// for any long-running work.
//
func Make(peers []*labrpc.ClientEnd, me int,
	persister *Persister, applyCh chan ApplyMsg) *Raft {
	rf := &Raft{}
	rf.peers = peers
	rf.persister = persister
	rf.me = me
	rf.applyCH = applyCh

	// Your initialization code here (2A, 2B, 2C).

	//Persistent state
	rf.currentTerm = 0
	rf.votedFor = -1
	rf.log = MakeLog(0, 0)

	//Volatile state on all servers
	rf.commitIndex = 0
	rf.lastApplied = 0

	//Volatile state on leaders
	rf.nextIndex = make([]int, len(peers))
	rf.matchIndex = make([]int, len(peers))

	//Other Volatile State
	rf.state = follower
	rf.leaderID = -1
	rf.gatheredVots = 0
	rf.lastElectionActive = time.Now()
	rf.lastAppendEntries = time.Now()

	// Some constants
	rf.peersNum = len(peers)
	rf.majorityNum = (rf.peersNum + 1) / 2
	rf.requestVoteTimeoutMin = time.Millisecond * 400
	rf.requestVoteTimeoutMax = time.Millisecond * 600
	rf.appendEntriesInterval = time.Millisecond * 120

	// notify application when log size change
	rf.SnapshotCh = make(chan SnapShotMsg)

	// initialize from state persisted before a crash
	rf.readPersist(persister.ReadRaftState())

	go rf.periodicTask()

	rf.DPrintf("started")

	return rf
}
