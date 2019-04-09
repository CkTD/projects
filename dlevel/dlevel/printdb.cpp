#include <cassert>
#include <iostream>
#include "leveldb/db.h"


// print all key-value pair in leveldb

int main(int argc, char* argv[])
{
    if (argc!=2)
    {
        std::cerr << "pdb dbpath\n";
        exit(1);
    }
    
    // open the db
    leveldb::DB* db;
    leveldb::Status s;
    leveldb::Options options;
    options.create_if_missing = false;
    s = leveldb::DB::Open(options,argv[1], &db);
    if (!s.ok()) 
    {
        std::cerr << "Can't open db" << s.ToString() << std::endl;
        //assert(s.ok());
        return -1;
    }
    // Iteration
    leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::cout << it->key().ToString() << ": "  << it->value().ToString() << std::endl;
    }
    assert(it->status().ok());  // Check for any errors found during the scan
    delete it;

    // close the db
    delete db;

    return 0;
}