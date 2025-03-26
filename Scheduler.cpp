//
//  Scheduler.cpp
//  CloudSim
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 10/20/24.
//

#include "Scheduler.hpp"

static bool migrating = false;
static unsigned active_machines = 16;
static int total  =0;

void Scheduler::Init() {
    // Find the parameters of the clusters
    // Get the total number of machines
    // For each machine:
    //      Get the type of the machine
    //      Get the memory of the machine
    //      Get the number of CPUs
    //      Get if there is a GPU or not
    // 
    SimOutput("Scheduler::Init(): Total number of machines is " + to_string(Machine_GetTotal()), 0);
    SimOutput("Scheduler::Init(): Initializing scheduler", 1);
    for(unsigned int i = 0; i < Machine_GetTotal(); ++i){
        MachineInfo_t info = Machine_GetInfo(i);
        machines.push_back(make_pair(i, 0));
        vms_map[i] = vector<VMId_t>();
    }
}

//helper function to sort by second pair value, in this algorithm this represents insturction count (which approximates utlization/workload).
void Scheduler::sortMachines(vector<pair<MachineId_t, unsigned int>>& vec) {
    sort(vec.begin(), vec.end(), [](const pair<MachineId_t, unsigned int>& a, const pair<MachineId_t, unsigned int>& b) {
        return a.second < b.second;
    });
}

void Scheduler::MigrationComplete(Time_t time, VMId_t vm_id) {
    // Update your data structure. The VM now can receive new tasks
}


void Scheduler::NewTask(Time_t now, TaskId_t task_id) {
    // Get the task parameters
    //  IsGPUCapable(task_id);
    //  GetMemory(task_id);
    //  RequiredVMType(task_id);
    //  RequiredSLA(task_id);
    //  RequiredCPUType(task_id);
    // Decide to attach the task to an existing VM, 
    //      vm.AddTask(taskid, Priority_T priority); or
    // Create a new VM, attach the VM to a machine
    //      VM vm(type of the VM)
    //      vm.Attach(machine_id);
    //      vm.AddTask(taskid, Priority_t priority) or
    // Turn on a machine, create a new VM, attach it to the VM, then add the task
    //
    // Turn on a machine, migrate an existing VM from a loaded machine....
    //
    // Other possibilities as desired

    //if no vms, find first compatible machine and add.

    TaskInfo_t t_info = GetTaskInfo(task_id);
    Priority_t priority = LOW_PRIORITY;
    if(t_info.required_sla == SLA0){
        priority = HIGH_PRIORITY;
    }
    if(t_info.required_sla == SLA1 || t_info.required_sla == SLA2){
        priority = MID_PRIORITY;
    }
    if(vms.size() == 0){
        bool added = false;
        for(int i =0; i < machines.size(); ++i){
            VMId_t vm = createVMforTask(task_id, machines[i].first);
            if(vm != UINT_MAX){
                added = true;
                VM_AddTask(vm, task_id, priority);
                machines[i].second += GetTaskInfo(task_id).total_instructions/1000000;
                sortMachines(machines);
                task_map[task_id] = machines[i].first;
                return;
            }
        }
        assert(added);
    }
    
    for(int i = 0; i < machines.size(); ++i){
        bool added = false;
        MachineInfo_t m_info = Machine_GetInfo(machines[i].first);
        for(int j = 0; j < vms_map[machines[i].first].size(); ++j){
            VMInfo_t v_info = VM_GetInfo(vms_map[machines[i].first][j]);
            
            if(v_info.cpu == t_info.required_cpu && v_info.vm_type == t_info.required_vm && m_info.memory_size-m_info.memory_used > t_info.required_memory){
                added = true;
                VM_AddTask(vms_map[machines[i].first][j], task_id, priority);
                machines[i].second += GetTaskInfo(task_id).total_instructions/1000000;
                sortMachines(machines);
                task_map[task_id] = machines[i].first;
                return;
            }
        }
        if(!added){
            VMId_t vm = createVMforTask(task_id, machines[i].first);
            if(vm!=UINT_MAX){
                VM_AddTask(vm, task_id, priority);
                machines[i].second += GetTaskInfo(task_id).total_instructions/1000000;
                sortMachines(machines);
                task_map[task_id] = machines[i].first;
                return;
            }
        }
    }
    

}
VMId_t Scheduler::createVMforTask(TaskId_t task_id,  MachineId_t machine_id){
    TaskInfo_t t_info = GetTaskInfo(task_id);
    VMId_t vm = VM_Create(t_info.required_vm , t_info.required_cpu);
    MachineInfo_t m_info = Machine_GetInfo(machine_id);
    if(m_info.cpu == t_info.required_cpu && m_info.memory_size - VM_MEMORY_OVERHEAD > t_info.required_memory){
        vms_map[machine_id].push_back(vm);
        
        VM_Attach(vm, machine_id);
        vms.push_back(vm);
        return vm;
    }
    return -1;
}

void Scheduler::PeriodicCheck(Time_t now) {
    // This method should be called from SchedulerCheck()
    // SchedulerCheck is called periodically by the simulator to allow you to monitor, make decisions, adjustments, etc.
    // Unlike the other invocations of the scheduler, this one doesn't report any specific event
    // Recommendation: Take advantage of this function to do some monitoring and adjustments as necessary
}

void Scheduler::Shutdown(Time_t time) {
    // Do your final reporting and bookkeeping here.
    // Report about the total energy consumed
    // Report about the SLA compliance
    // Shutdown everything to be tidy :-)
    for(auto & vm: vms) {
        VM_Shutdown(vm);
    }
    SimOutput("SimulationComplete(): Finished!", 4);
    SimOutput("SimulationComplete(): Time is " + to_string(time), 4);
}

void Scheduler::TaskComplete(Time_t now, TaskId_t task_id) {
    // Do any bookkeeping necessary for the data structures
    // Decide if a machine is to be turned off, slowed down, or VMs to be migrated according to your policy
    // This is an opportunity to make any adjustments to optimize performance/energy
    SimOutput("Scheduler::TaskComplete(): Task " + to_string(task_id) + " is complete at " + to_string(now), 4);

    TaskInfo_t t_info = GetTaskInfo(task_id);
    
    for (int i =0; i < machines.size(); ++i) {
        if (machines[i].first == task_map[task_id]) {
            if(t_info.total_instructions/1000000 >machines[i].second ){
                machines[i].second = 0;
            }
            else{
                machines[i].second -= t_info.total_instructions/1000000;
            }
            break; 
        }
    }
    task_map.erase(task_id);
    // for (const auto& machine : machines) {
    //     std::cout << "MachineId: " << machine.first << ", Value: " << machine.second << std::endl;
    // }
}

// Public interface below

static Scheduler Scheduler;

void InitScheduler() {
    SimOutput("InitScheduler(): Initializing scheduler", 4);
    Scheduler.Init();
}

void HandleNewTask(Time_t time, TaskId_t task_id) {
    SimOutput("HandleNewTask(): Received new task " + to_string(task_id) + " at time " + to_string(time), 4);
    Scheduler.NewTask(time, task_id);
}

void HandleTaskCompletion(Time_t time, TaskId_t task_id) {
    SimOutput("HandleTaskCompletion(): Task " + to_string(task_id) + " completed at time " + to_string(time), 4);
    Scheduler.TaskComplete(time, task_id);
}

void MemoryWarning(Time_t time, MachineId_t machine_id) {
    // The simulator is alerting you that machine identified by machine_id is overcommitted
    SimOutput("MemoryWarning(): Overflow at " + to_string(machine_id) + " was detected at time " + to_string(time), 1);
}

void MigrationDone(Time_t time, VMId_t vm_id) {
    // The function is called on to alert you that migration is complete
    SimOutput("MigrationDone(): Migration of VM " + to_string(vm_id) + " was completed at time " + to_string(time), 4);
    Scheduler.MigrationComplete(time, vm_id);
    migrating = false;
}

void SchedulerCheck(Time_t time) {
    // This function is called periodically by the simulator, no specific event
    SimOutput("SchedulerCheck(): SchedulerCheck() called at " + to_string(time), 4);
    Scheduler.PeriodicCheck(time);

}

void SimulationComplete(Time_t time) {
    // This function is called before the simulation terminates Add whatever you feel like.
    cout << "SLA violation report" << endl;
    cout << "SLA0: " << GetSLAReport(SLA0) << "%" << endl;
    cout << "SLA1: " << GetSLAReport(SLA1) << "%" << endl;
    cout << "SLA2: " << GetSLAReport(SLA2) << "%" << endl;     // SLA3 do not have SLA violation issues
    cout << "Total Energy " << Machine_GetClusterEnergy() << "KW-Hour" << endl;
    cout << "Simulation run finished in " << double(time)/1000000 << " seconds" << endl;
    SimOutput("SimulationComplete(): Simulation finished at time " + to_string(time), 4);

    Scheduler.Shutdown(time);
}

void SLAWarning(Time_t time, TaskId_t task_id) {
    
}

void StateChangeComplete(Time_t time, MachineId_t machine_id) {
    // Called in response to an earlier request to change the state of a machine
}

