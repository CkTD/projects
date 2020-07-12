package mr

import (
	"encoding/json"
	"fmt"
	"hash/fnv"
	"io/ioutil"
	"log"
	"net/rpc"
	"os"
	"sort"
	"strconv"
	"time"
)

// KeyValue Map functions return a slice of KeyValue.
type KeyValue struct {
	Key   string
	Value string
}

// ByKey for sorting by key.
type ByKey []KeyValue

// for sorting by key.
func (a ByKey) Len() int           { return len(a) }
func (a ByKey) Swap(i, j int)      { a[i], a[j] = a[j], a[i] }
func (a ByKey) Less(i, j int) bool { return a[i].Key < a[j].Key }

//
// use ihash(key) % NReduce to choose the reduce
// task number for each KeyValue emitted by Map.
//
func ihash(key string) int {
	h := fnv.New32a()
	h.Write([]byte(key))
	return int(h.Sum32() & 0x7fffffff)
}

// Worker main/mrworker.go calls this function.
func Worker(mapf func(string, string) []KeyValue,
	reducef func(string, []string) string) {

	// Your worker implementation here.

	// uncomment to send the Example RPC to the master.
	// CallExample()

	for {
		atr, err := callAskTask()
		if err != nil {
			fmt.Println(err)
			return
		}
		switch atr.TaskType {
		case TaskNone:
			return
		case TaskWait:
			time.Sleep(time.Second)
		case TaskMap:
			doMap(mapf, atr.File, atr.NReduce, atr.No)
			callFinishTask(TaskMap, atr.No)
		case TaskReduce:
			doReduce(reducef, atr.NMap, atr.No)
			callFinishTask(TaskReduce, atr.No)
		}
	}
}

func doMap(mapf func(string, string) []KeyValue, filename string, nReduce int, no int) {
	f, err := os.Open(filename)
	if err != nil {
		log.Fatalf("cannot open %v", filename)
	}
	content, err := ioutil.ReadAll(f)
	if err != nil {
		log.Fatalf("cannot read %v", filename)
	}
	f.Close()
	kva := mapf(filename, string(content))

	encs := make(map[int]*json.Encoder)
	for i := 0; i < nReduce; i++ {
		ifn := "mr-" + strconv.Itoa(no) + "-" + strconv.Itoa(i)
		f, err := os.Create(ifn)
		if err != nil {
			log.Fatalf("Map: cannot create intermediate file %v", ifn)
		}
		defer f.Close()
		encs[i] = json.NewEncoder(f)
	}
	for _, kv := range kva {
		bucket := ihash(kv.Key) % nReduce
		err := encs[bucket].Encode(&kv)
		if err != nil {
			panic("Encod Error")
		}
	}
}

func doReduce(reducef func(string, []string) string, nMap int, no int) {
	var kva []KeyValue
	for i := 0; i < nMap; i++ {
		ifn := "mr-" + strconv.Itoa(i) + "-" + strconv.Itoa(no)
		f, err := os.Open(ifn)
		if err != nil {
			log.Fatalf("Reduce: cannot open intermediate file %v", ifn)
		}
		defer f.Close()
		dec := json.NewDecoder(f)
		for {
			var kv KeyValue
			err := dec.Decode(&kv)
			if err != nil {
				break
			}
			kva = append(kva, kv)
		}
	}

	sort.Sort(ByKey(kva))

	oname := "mr-out-" + strconv.Itoa(no)
	ofile, _ := os.Create(oname)
	defer ofile.Close()

	// call Reduce on each distinct key in intermediate[],
	// and print the result to mr-out-0.
	i := 0
	for i < len(kva) {
		j := i + 1
		for j < len(kva) && kva[j].Key == kva[i].Key {
			j++
		}
		values := []string{}
		for k := i; k < j; k++ {
			values = append(values, kva[k].Value)
		}
		output := reducef(kva[i].Key, values)

		// this is the correct format for each line of Reduce output.
		fmt.Fprintf(ofile, "%v %v\n", kva[i].Key, output)

		i = j
	}

}

func callAskTask() (AskTaskReply, error) {
	// declare an argument structure.
	args := AskTaskArgs{}

	// declare a reply structure.
	reply := AskTaskReply{}

	// send the RPC request, wait for the reply.
	err := call("Master.AskTask", &args, &reply)

	return reply, err
}

func callFinishTask(taskType TaskType, no int) error {
	args := FinishTaskArgs{TaskType: taskType, No: no}

	reply := FinishTaskReply{}

	return call("Master.FinishTask", &args, &reply)
}

//
// send an RPC request to the master, wait for the response.
// usually returns true.
// returns false if something goes wrong.
//
func call(rpcname string, args interface{}, reply interface{}) error {
	// c, err := rpc.DialHTTP("tcp", "127.0.0.1"+":1234")
	sockname := masterSock()
	c, err := rpc.DialHTTP("unix", sockname)
	if err != nil {
		log.Fatal("dialing:", err)
		return err
	}
	defer c.Close()

	err = c.Call(rpcname, args, reply)
	if err != nil {
		fmt.Println("rpc return an error", err)
		return err
	}
	return nil
}
