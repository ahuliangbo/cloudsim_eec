//
//  Scheduler.cpp
//  CloudSim
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 10/20/24.
//

#include "Scheduler.hpp"

static bool migrating = false;
static unsigned active_machines = 16;

void Scheduler::Init() {
    // Find the parameters of the clusters
    // Get the total number of machines
    // For each machine:
    //      Get the type of the machine
    //      Get the memory of the machine
    //      Get the number of CPUs
    //      Get if there is a GPU or not
    // 
    SimOutput("Scheduler::Init(): Total number of machines is " + to_string(Machine_GetTotal()), 3);
    SimOutput("Scheduler::Init(): Initializing scheduler", 1);
    for(int i =0; i < Machine_GetTotal(); ++i){
        machines.push_back(MachineId_t(i));
        machines_map[machines[i]] = 0;
        VMId_t vm = VM_Create(LINUX, Machine_GetInfo(machines[i]).cpu);
        // vms.push_back(vm);
        machines_vms_map[machines[i]].push_back(vm);
        VM_Attach(vm, machines[i]);
      
    }
    // for (const auto& pair : machines_vms_map) {
    //     std::cout << "Machine ID: " << pair.first << " -> VMs: ";
    //     if (pair.second.empty()) {
    //         std::cout << "No VMs";
    //     } else {
    //         for (const auto& vm : pair.second) {
    //             std::cout << vm << " ";
    //         }
    //     }
    //     std::cout << std::endl;
    // }
    // for( auto machine:machines){
    //     MachineInfo_t inf = Machine_GetInfo(machine);

    //     cout << inf.active_vms << endl;
    // }
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
    TaskInfo_t t_info = GetTaskInfo(task_id);
    Priority_t priority = (t_info.required_sla != SLAType_t::SLA3)? MID_PRIORITY : LOW_PRIORITY;

    
    //chatgpt comparator and minheap
    auto cmp = [](const std::pair<unsigned, unsigned>& a, const std::pair<unsigned, unsigned>& b) {
        return a.second > b.second; // Min-heap based on second element
    };
    std::priority_queue<std::pair<unsigned, unsigned>, std::vector<std::pair<unsigned, unsigned>>, decltype(cmp)> minHeap(cmp);

    for(int i =0; i < Machine_GetTotal(); ++i){
        bool added = false;
        MachineInfo_t m_info = Machine_GetInfo(machines[i]);
        unsigned int ETA = machines_map[machines[i]]/m_info.performance[m_info.p_state]+ Now();
        minHeap.push({ machines[i],ETA });
    }
    //get top (could change to be only nonzero tops)
    bool added = false;
    while(!added){
        MachineId_t BestFit = minHeap.top().first;
        minHeap.pop();
        MachineInfo_t m_info = Machine_GetInfo(BestFit);
        unsigned int b_ETA = machines_map[BestFit]/m_info.performance[m_info.p_state]+ Now();
        if(m_info.memory_size - m_info.memory_used - t_info.required_memory - VM_MEMORY_OVERHEAD < 0 && !minHeap.empty()){
            continue;
        }
        vector<VMId_t> machine_vms = machines_vms_map[BestFit];
        // assert(machine_vms.size() == m_info.active_vms);
        for(int i =0; i < machine_vms.size(); ++i){
            VMInfo_t v_info = VM_GetInfo(machine_vms[i]);
            if( t_info.required_cpu== v_info.cpu && t_info.required_vm == v_info.vm_type){
                added = true;
                //add task, update data structures
                //if eta vs required time ratio is too low
                if(b_ETA / t_info.target_completion > 1.1){
                     priority = HIGH_PRIORITY;
                }
                
                AddTask(task_id, machine_vms[i], priority);
            }
        }
        if(m_info.active_vms == 0 || added == false && t_info.required_cpu == m_info.cpu){
            added = true;
            //make vm and add
            VMId_t vm = VM_Create( t_info.required_vm, t_info.required_cpu);
            VM_Attach(vm, BestFit);
            machines_vms_map[BestFit].push_back(vm);
            // vms.pushback();

            if(b_ETA / t_info.target_completion > 1.01){
                priority = HIGH_PRIORITY;
            }
            AddTask(task_id, vm, priority);
        }
    }
    //bandaid last resort
    if(!added){
        for(int i =0; i < Machine_GetTotal(); ++i){
            MachineInfo_t m_info = Machine_GetInfo(machines[i]);
            if(t_info.required_cpu == m_info.cpu){
                            //make vm and add
                VMId_t vm = VM_Create( t_info.required_vm, t_info.required_cpu);
                VM_Attach(vm, machines[i]);
                machines_vms_map[machines[i]].push_back(vm);
                // vms.pushback();

                priority = HIGH_PRIORITY;
                
                AddTask(task_id, vm, priority);
            }
        }
    }
}
void Scheduler::AddTask(TaskId_t task_id, VMId_t vm_id, Priority_t priority) {
    VMInfo_t v_info = VM_GetInfo(vm_id);
    TaskInfo_t t_info = GetTaskInfo(task_id);
    VM_AddTask(vm_id, task_id, priority);
    tasks[task_id] = vm_id;
    machines_map[v_info.machine_id] += t_info.total_instructions/1000000;
}
void Scheduler::RemoveTask(TaskId_t task_id, VMId_t vm_id) {
    VMInfo_t v_info = VM_GetInfo(vm_id);
    TaskInfo_t t_info = GetTaskInfo(task_id);
    tasks.erase(task_id);
    machines_map[v_info.machine_id] = machines_map[v_info.machine_id]<t_info.total_instructions/1000000 ?
    0 : machines_map[v_info.machine_id]-t_info.total_instructions/1000000;
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
    RemoveTask(task_id, tasks[task_id]);
    SimOutput("Scheduler::TaskComplete(): Task " + to_string(task_id) + " is complete at " + to_string(now), 4);
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

