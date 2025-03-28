//
//  Scheduler.hpp
//  CloudSim
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 10/20/24.
//

#ifndef Scheduler_hpp
#define Scheduler_hpp

#include <vector>

#include "Interfaces.h"
#include <bits/stdc++.h>

class Scheduler {
public:
    Scheduler()                 {}
    void Init();
    void MigrationComplete(Time_t time, VMId_t vm_id);
    void NewTask(Time_t now, TaskId_t task_id);
    void PeriodicCheck(Time_t now);
    void Shutdown(Time_t now);
    void TaskComplete(Time_t now, TaskId_t task_id);
    void AddTask(TaskId_t task_id, VMId_t vm_id, Priority_t priority);
    void RemoveTask(TaskId_t task_id, VMId_t vm_id);
private:
    vector<VMId_t> vms;
    map<TaskId_t, VMId_t> tasks;
    map<MachineId_t, unsigned int> machines_map;
    map<MachineId_t, vector<VMId_t>> machines_vms_map;
    vector<MachineId_t> machines;
};



#endif /* Scheduler_hpp */
