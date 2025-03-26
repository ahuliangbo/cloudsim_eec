//
//  Scheduler.hpp
//  CloudSim
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 10/20/24.
//

#ifndef Scheduler_hpp
#define Scheduler_hpp

#include <vector>
#include <bits/stdc++.h>
#include "Interfaces.h"

class Scheduler {
public:
    Scheduler()                 {}
    void Init();
    void MigrationComplete(Time_t time, VMId_t vm_id);
    void NewTask(Time_t now, TaskId_t task_id);
    void PeriodicCheck(Time_t now);
    void Shutdown(Time_t now);
    void TaskComplete(Time_t now, TaskId_t task_id);
    vector<pair<MachineId_t, unsigned int>> machines;
    map<MachineId_t, vector<VMId_t>> vms_map;
    map<TaskId_t, MachineId_t> task_map;
private:
    vector<VMId_t> vms;
    void sortMachines(vector<pair<MachineId_t, unsigned int>>& vec);
    VMId_t createVMforTask(TaskId_t task_id, MachineId_t machine_id);
};



#endif /* Scheduler_hpp */
