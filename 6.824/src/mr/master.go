package mr

import (
	"log"
	"net"
	"net/http"
	"net/rpc"
	"os"
	"time"
)

type statusType int8

const (
	statusPending statusType = 0
	statusWorking statusType = 1
	statusDone    statusType = 2
)

// Master this is master
type Master struct {
	// Your definitions here.
	files        []string
	nReduce      int
	nMap         int
	mapDone      bool
	reduceDone   bool
	mapStatus    []statusType
	reduceStatus []statusType
}

// Your code here -- RPC handlers for the worker to call.

//AskTask ask task RPC handler
func (m *Master) AskTask(args *AskTaskArgs, reply *AskTaskReply) error {
	if !m.mapDone {
		for i, s := range m.mapStatus {
			if s == statusPending {
				reply.TaskType = TaskMap
				reply.No = i
				reply.NReduce = m.nReduce
				reply.NMap = m.nMap
				reply.File = m.files[i]
				m.mapStatus[i] = statusWorking
				go func() {
					<-time.NewTimer(time.Second * 10).C
					if m.mapStatus[i] == statusWorking {
						m.mapStatus[i] = statusPending
					}
				}()
				return nil
			}
		}

		reply.TaskType = TaskWait
		return nil
	} else if !m.reduceDone {
		for i, s := range m.reduceStatus {
			if s == statusPending {
				reply.TaskType = TaskReduce
				reply.No = i
				reply.NReduce = m.nReduce
				reply.NMap = m.nMap
				reply.File = ""
				m.reduceStatus[i] = statusWorking
				go func() {
					<-time.NewTimer(time.Second * 10).C
					if m.reduceStatus[i] == statusWorking {
						m.reduceStatus[i] = statusPending
					}
				}()
				return nil
			}
		}
		reply.TaskType = TaskWait
		return nil
	} else {
		reply.TaskType = TaskNone
		return nil
	}
}

//FinishTask finish task RPC handler
func (m *Master) FinishTask(args *FinishTaskArgs, reply *FinishTaskReply) error {
	if args.TaskType == TaskMap && m.mapStatus[args.No] == statusWorking {
		m.mapStatus[args.No] = statusDone

		mapDone := true
		for _, s := range m.mapStatus {
			if s != statusDone {
				mapDone = false
			}
		}
		m.mapDone = mapDone
	} else if args.TaskType == TaskReduce && m.reduceStatus[args.No] == statusWorking {
		m.reduceStatus[args.No] = statusDone

		reduceDone := true
		for _, s := range m.reduceStatus {
			if s != statusDone {
				reduceDone = false
			}
		}
		m.reduceDone = reduceDone
	}
	return nil
}

//
// start a thread that listens for RPCs from worker.go
//
func (m *Master) server() {
	rpc.Register(m)
	rpc.HandleHTTP()
	//l, e := net.Listen("tcp", ":1234")
	sockname := masterSock()
	os.Remove(sockname)
	l, e := net.Listen("unix", sockname)
	if e != nil {
		log.Fatal("listen error:", e)
	}
	go http.Serve(l, nil)
}

// Done main/mrmaster.go calls Done() periodically to find out
// if the entire job has finished.
func (m *Master) Done() bool {
	// Your code here.
	return m.mapDone && m.reduceDone
}

// MakeMaster create a Master.
// main/mrmaster.go calls this function.
// nReduce is the number of reduce tasks to use.
func MakeMaster(files []string, nReduce int) *Master {
	// Your code here.
	m := Master{
		files:        files,
		nReduce:      nReduce,
		nMap:         len(files),
		mapDone:      false,
		reduceDone:   false,
		mapStatus:    make([]statusType, len(files)),
		reduceStatus: make([]statusType, nReduce),
	}

	m.server()
	return &m
}
