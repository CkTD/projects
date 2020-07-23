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
}

//
// Log entry
//
type LogEntry struct {
	Term    int
	Command interface{}
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
	log         []LogEntry

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
}

// caller lock the mutex
func (rf *Raft) getLastLogTerm() int {
	return rf.log[len(rf.log)-1].Term
}

// caller lock the mutex
func (rf *Raft) getLastLogIndex() int {
	return len(rf.log) - 1
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

//
// save Raft's persistent state to stable storage,
// where it can later be retrieved after a crash and restart.
// see paper's Figure 2 for a description of what should be persistent.
//
// caller lock the mutex
func (rf *Raft) persist() {
	// Your code here (2C).
	w := new(bytes.Buffer)
	e := labgob.NewEncoder(w)
	e.Encode(rf.currentTerm)
	e.Encode(rf.votedFor)
	e.Encode(rf.log)
	data := w.Bytes()
	rf.persister.SaveRaftState(data)
}

//
// restore previously persisted state.
//
// caller lock the mutex
func (rf *Raft) readPersist(data []byte) {
	if data == nil || len(data) < 1 { // bootstrap without any state?
		return
	}
	// Your code here (2C).
	r := bytes.NewBuffer(data)
	d := labgob.NewDecoder(r)
	var currentTerm int
	var voteFor int
	var log []LogEntry
	if d.Decode(&currentTerm) != nil ||
		d.Decode(&voteFor) != nil ||
		d.Decode(&log) != nil {
		panic("read persist error")
	} else {
		DPrintf(rf, "load persist state, Term:[%d], voteFor:[%d], last log index: [%d]", currentTerm, voteFor, len(log)-1)
		rf.currentTerm = currentTerm
		rf.votedFor = voteFor
		rf.log = log
		rf.matchIndex[rf.me] = rf.getLastLogIndex()
	}
}

func (rf *Raft) findLog(term int, index int) {

}

//
// example RequestVote RPC arguments structure.
// field names must start with capital letters!
//
type RequestVoteArgs struct {
	// Your data here (2A, 2B).
	Term         int
	CandidateID  int
	LastLogIndex int
	LastLogTerm  int
}

//
// example RequestVote RPC reply structure.
// field names must start with capital letters!
//
type RequestVoteReply struct {
	// Your data here (2A).
	Term        int
	VoteGranted bool
}

//
// example RequestVote RPC handler.
//
func (rf *Raft) RequestVote(args *RequestVoteArgs, reply *RequestVoteReply) {
	// Your code here (2A, 2B).
	rf.mu.Lock()
	reply.Term = rf.currentTerm
	reply.VoteGranted = false

	// follow up if necessary
	if args.Term > rf.currentTerm {
		rf.currentTerm = args.Term
		rf.state = follower
		rf.votedFor = -1
	}

	// vote if possible
	if args.Term == rf.currentTerm && // The candidate must have latest term
		(rf.votedFor == -1 || rf.votedFor == args.CandidateID) && // We havn't vote any other peer in this term
		(args.LastLogTerm > rf.getLastLogTerm() || // The candidate's log is as up-to-date as ours
			(args.LastLogTerm == rf.getLastLogTerm() && args.LastLogIndex >= rf.getLastLogIndex())) {
		rf.votedFor = args.CandidateID
		reply.Term = rf.currentTerm
		reply.VoteGranted = true
		rf.lastElectionActive = time.Now()
		DPrintf(rf, "vote for [%d]", args.CandidateID)
	}

	rf.persist()

	rf.mu.Unlock()
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
	if args.Term < rf.currentTerm { // leader already out of date
		reply.Success = false
		reply.Term = rf.currentTerm
	} else { // leader is valid for us
		// follow up if necessary
		rf.lastElectionActive = time.Now()
		rf.currentTerm = args.Term
		rf.leaderID = args.LeaderID
		rf.state = follower
		reply.Term = args.Term
		if args.Term > rf.currentTerm || (args.Term == rf.currentTerm && rf.leaderID == -1) {
			DPrintf(rf, "new leader[%d] detected", args.LeaderID)
		}

		if rf.getLastLogIndex() >= args.PrevLogIndex &&
			rf.log[args.PrevLogIndex].Term == args.PrevLogTerm {
			// Success, we have the log whose (index,term) match leader's expectation (PrevLogIndex, PrevLogTerm)
			reply.Success = true

			// RPCs are idempotent
			// if a follower receives an AE request that includes log entries already present in its log, it ignores those entries in the new request.
			if len(args.Entries) != 0 &&
				(rf.getLastLogIndex() >= args.PrevLogIndex+len(args.Entries) &&
					rf.log[args.PrevLogIndex+len(args.Entries)].Term == args.Entries[len(args.Entries)-1].Term) {
			} else {
				if rf.getLastLogIndex() > args.PrevLogIndex { // Overwrite conflict logs
					DPrintf(rf, "AE: overwrite conflict logs, PrevLogIndex, PrevLogTerm: (%d,%d). LastLogIndex: %d ", args.PrevLogIndex, args.PrevLogTerm, rf.getLastLogIndex())
					rf.log = rf.log[:args.PrevLogIndex+1]
				}
				if len(args.Entries) != 0 {
					DPrintf(rf, "AE: %d logs received [%d]-[%d]", len(args.Entries), rf.getLastLogIndex()+1, rf.getLastLogIndex()+len(args.Entries))
					rf.log = append(rf.log, args.Entries...)
				}
			}

			if args.LeaderCommit > rf.commitIndex {
				DPrintf(rf, "commit log [%d]-[%d]", rf.commitIndex+1, args.LeaderCommit)
				rf.commitIndex = Min(args.LeaderCommit, rf.getLastLogIndex())
			}
		} else { // Failed, exist further confliction
			reply.Success = false
			if rf.getLastLogIndex() >= args.PrevLogIndex {
				reply.XLen = args.PrevLogIndex
				reply.XTerm = rf.log[args.PrevLogIndex].Term
				reply.XIndex = args.PrevLogIndex
				for rf.log[reply.XIndex-1].Term == reply.XTerm {
					reply.XIndex--
				}
			} else {
				reply.XLen = rf.getLastLogIndex()
				reply.XTerm = rf.log[rf.getLastLogIndex()].Term
				reply.XIndex = rf.getLastLogIndex()
				for reply.XIndex-1 >= 0 && rf.log[reply.XIndex-1].Term == reply.XTerm {
					reply.XIndex--
				}
			}
		}
	}
	rf.persist()
	rf.mu.Unlock()
}

func (rf *Raft) sendAppendEntries(server int, args *AppendEntriesArgs, reply *AppendEntriesReply) bool {
	ok := rf.peers[server].Call("Raft.AppendEntries", args, reply)
	return ok
}

func (rf *Raft) Start(command interface{}) (int, int, bool) {
	index := -1
	term := -1
	isLeader := true
	// Your code here (2B).
	rf.mu.Lock()
	isLeader = rf.state == leader
	if isLeader {
		rf.log = append(rf.log, LogEntry{Term: rf.currentTerm, Command: command})
		rf.matchIndex[rf.me] = rf.getLastLogIndex()

		index = rf.getLastLogIndex()
		term = rf.currentTerm
		rf.persist() // for follower, persist is necessary for new log
		DPrintf(rf, "start a new command, Index[%d]", rf.getLastLogIndex())
	}
	rf.mu.Unlock()

	return index, term, isLeader
}

//
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
}

func (rf *Raft) killed() bool {
	z := atomic.LoadInt32(&rf.dead)
	return z == 1
}

func (rf *Raft) periodicTask() {
	for {
		time.Sleep(time.Millisecond * 20)

		rf.mu.Lock()
		rf.applyLogHandler()    // all servers apply committed logs
		if rf.state == leader { // leader send appendEntries periodically
			if time.Since(rf.lastAppendEntries) > rf.appendEntriesInterval {
				DPrintf(rf, "send append entries")
				rf.lastAppendEntries = time.Now()
				rf.appendEntryTimeoutHandler()
			}
		} else { // candidate and follower check election timeout periodically
			timeout := RandIntRange(rf.requestVoteTimeoutMin, rf.requestVoteTimeoutMax)
			if time.Since(rf.lastElectionActive) > timeout {
				DPrintf(rf, "election timeout")
				rf.lastElectionActive = time.Now()
				rf.electionTimeoutHandler()
			}
		}
		rf.mu.Unlock()
	}
}

// caller lock the mutex
// return immediately
func (rf *Raft) applyLogHandler() {
	for rf.commitIndex > rf.lastApplied {
		rf.lastApplied++
		msg := ApplyMsg{
			CommandValid: true,
			Command:      rf.log[rf.lastApplied].Command,
			CommandIndex: rf.lastApplied}
		rf.persist()
		DPrintf(rf, "apply log[%d]", msg.CommandIndex)
		rf.applyCH <- msg
	}
}

// caller Lock the mutex
// return immediately
func (rf *Raft) appendEntryTimeoutHandler() {
	term := rf.currentTerm
	for s := range rf.peers {
		if s == rf.me {
			continue
		}
		go func(s int) {

			rf.mu.Lock()
			// check we are still leader.
			// after the lock is acquired, the state may have changed, checke it
			if rf.state != leader || rf.currentTerm != term {
				rf.mu.Unlock()
				return
			}
			l := rf.log[rf.nextIndex[s] : rf.getLastLogIndex()+1]
			args := AppendEntriesArgs{
				Term:         rf.currentTerm,
				LeaderID:     rf.me,
				PrevLogIndex: rf.nextIndex[s] - 1,
				PrevLogTerm:  rf.log[rf.nextIndex[s]-1].Term,
				Entries:      l,
				LeaderCommit: rf.commitIndex,
			}
			reply := AppendEntriesReply{}
			rf.mu.Unlock()

			ok := rf.sendAppendEntries(s, &args, &reply)

			rf.mu.Lock()
			if !ok || args.Term != rf.currentTerm { // ignore failed RPC & reply from old term
			} else if reply.Term > rf.currentTerm { // we are left behind -> follow up
				DPrintf(rf, "new term[%d] detected", reply.Term)
				rf.state = follower
				rf.leaderID = -1
				rf.currentTerm = reply.Term
				rf.lastElectionActive = time.Now()
			} else if reply.Success { // AE success -> try commit
				if len(l) > 0 && rf.matchIndex[s] >= args.PrevLogIndex+len(l) {
					DPrintf(rf, fmt.Sprintf("outdated AE reply, current matchIndex[%d] for follower[%d], for log[%d]-[%d]", rf.matchIndex[s], s, args.PrevLogIndex+1, args.PrevLogIndex+len(args.Entries)))
				}
				if len(l) > 0 && (rf.matchIndex[s] < args.PrevLogIndex+len(l)) { // ignore reordered reply
					DPrintf(rf, "AE success for follower[%d], matchIndex[%d]->[%d]", s, rf.matchIndex[s], args.PrevLogIndex+len(l))
					rf.nextIndex[s] = args.PrevLogIndex + len(l) + 1
					rf.matchIndex[s] = rf.nextIndex[s] - 1

					if rf.nextIndex[s] > rf.getLastLogIndex()+1 {
						panic("nextIndex greater than lastIndex")
					}

					// Check commit
					N := rf.getLastLogIndex()
					for N > rf.commitIndex {
						if rf.log[N].Term == rf.currentTerm {
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
						DPrintf(rf, "commit index [%d]->[%d]", rf.commitIndex, N)
					}
					rf.commitIndex = N
				}
			} else { // AE conflict -> decrease nextIndex
				DPrintf(rf, "AE: log[%d]-[%d] failed for follower [%d] with XIndex:%d XTerm:%d", args.PrevLogIndex+1, args.PrevLogIndex+len(l), s, reply.XIndex, reply.XTerm)
				if rf.log[reply.XIndex].Term != reply.XTerm {
					rf.nextIndex[s] = reply.XIndex
				} else {
					nextIndex := reply.XIndex
					for nextIndex <= reply.XLen && rf.log[nextIndex].Term == reply.XTerm {
						nextIndex++
					}
					rf.nextIndex[s] = nextIndex
				}
				if rf.nextIndex[s] > rf.getLastLogIndex() {
					panic("WHAT?")
				}
			}
			rf.mu.Unlock()
		}(s)
	}
}

// caller Lock the mutex
// return immediately
func (rf *Raft) electionTimeoutHandler() {
	rf.currentTerm++
	term := rf.currentTerm
	rf.state = candidate
	rf.votedFor = rf.me
	rf.gatheredVots = 1
	for s := range rf.peers {
		if s == rf.me {
			continue
		}
		go func(s int) {
			rf.mu.Lock()
			// after the lock is acquired, the state may have changed, check it
			if rf.state == leader || rf.currentTerm != term {
				rf.mu.Unlock()
				return
			}
			args := RequestVoteArgs{
				Term:         rf.currentTerm,
				CandidateID:  rf.me,
				LastLogTerm:  rf.getLastLogTerm(),
				LastLogIndex: rf.getLastLogIndex()}
			reply := RequestVoteReply{}
			rf.mu.Unlock()

			ok := rf.sendRequestVote(s, &args, &reply)

			rf.mu.Lock()
			if !ok || args.Term != rf.currentTerm { // ignore failed RPC & reply from old term
			} else if reply.Term > rf.currentTerm { // we are left behind, follow up
				DPrintf(rf, "new term[%d] detected", reply.Term)
				rf.state = follower
				rf.leaderID = -1
				rf.currentTerm = reply.Term
				rf.lastElectionActive = time.Now()
			} else if rf.state == candidate && // handle a valid vote -> try to be leader.
				reply.VoteGranted == true {
				rf.gatheredVots++
				if rf.gatheredVots >= rf.majorityNum {
					rf.state = leader
					for i := range rf.nextIndex {
						rf.nextIndex[i] = rf.getLastLogIndex() + 1
						rf.matchIndex[i] = 0
					}
					rf.matchIndex[rf.me] = rf.getLastLogIndex()
					DPrintf(rf, "become leader, lastLogIndex:%d", rf.getLastLogIndex())
				}
			}
			rf.mu.Unlock()
		}(s)
	}
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

	rf.currentTerm = 0
	rf.votedFor = -1
	rf.log = make([]LogEntry, 1) // The first LogEntry is for padding. The first index of real log is 1

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
	rf.requestVoteTimeoutMin = time.Millisecond * 200
	rf.requestVoteTimeoutMax = time.Millisecond * 400
	rf.appendEntriesInterval = time.Millisecond * 120

	// initialize from state persisted before a crash
	rf.readPersist(persister.ReadRaftState())

	go rf.periodicTask()

	return rf
}
