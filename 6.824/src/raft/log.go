package raft

//
// Log entry
//
type LogEntry struct {
	Term    int
	Command interface{}
}

// real log index starting from 1
// no concurrent call to Log's method
type Log struct {
	PrevIndex int
	PrevTerm  int
	Entries   []LogEntry
}

func (l *Log) LastIndex() int {
	return l.PrevIndex + len(l.Entries)
}

func (l *Log) LastTerm() int {
	n := len(l.Entries)
	if n == 0 {
		return l.PrevTerm
	}
	return l.Entries[n-1].Term
}

// return Logs index from start to end (including both)
// if end=-1 , return a logs after start(included)
// ok=false if start not in the log
// panic if end not in the log
func (l *Log) GetRange(start, end int) ([]LogEntry, bool) {
	if end > l.LastIndex() {
		panic("logs index out of range")
	}
	if start <= l.PrevIndex {
		return []LogEntry{}, false
	}
	if end == -1 {
		end = l.LastIndex()
	}

    res := l.Entries[start-l.PrevIndex-1 : end-l.PrevIndex]
    // make a copy of logs. 
    // append entries rpc read the returned logs but not protected by lock.
    dst := make([]LogEntry, len(res))
    copy(dst, res)
	return dst, true
}

func (l *Log) GetTerm(index int) int {
	if index == 0 {
		return 0
	}
	if index == l.PrevIndex {
		return l.PrevTerm
	}
	if index < l.PrevIndex || index > l.LastIndex() {
		panic("log index out of range")
	}
	return l.Entries[index-l.PrevIndex-1].Term
}

func (l *Log) GetCommand(index int) interface{} {
	if index <= l.PrevIndex || index > l.LastIndex() {
		panic("log index out of range")
	}
	return l.Entries[index-l.PrevIndex-1].Command
}

func (l *Log) Append(e ...LogEntry) {
	l.Entries = append(l.Entries, e...)
}

func (l *Log) DeleteAfter(index int) {
	if index > l.LastIndex() || index < l.PrevIndex {
		panic("log index out of range")
	}

	l.Entries = l.Entries[:index-l.PrevIndex]
}

func (l *Log) DiscardBefore(index int) {
	if index <= l.PrevIndex || index > l.LastIndex()+1 {
		panic("log index out of range")
	}
	if index == l.PrevIndex+1 {
		return
	}
	pIndex := index - 1
	pTerm := l.GetTerm(index - 1)
	l.Entries = l.Entries[pIndex-l.PrevIndex:]
	l.PrevIndex = pIndex
	l.PrevTerm = pTerm
}

const (
	exist     = 0
	notexist  = 1
	discarded = 2
)

func (l *Log) Find(term, index int) int {
	if index < l.PrevIndex {
		return discarded
	}

	if !(l.LastIndex() >= index &&
		l.GetTerm(index) == term) {
		return notexist
	}

	return exist
}

func MakeLog(prevTerm, prevIndex int) *Log {
	return &Log{PrevIndex: prevIndex, PrevTerm: prevTerm, Entries: make([]LogEntry, 0)}
}
