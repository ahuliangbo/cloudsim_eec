//
//  Scheduler.cpp
//  CloudSim
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 10/20/24.
//

#include "Scheduler.hpp"

static bool migrating = false;
static unsigned tasks_per_vm = 0; //universal constant to simplify, set in init

static unsigned long long instr = 0;
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
        machines_mm[machines[i]] = 0;
        tasks_per_vm+= Machine_GetInfo(MachineId_t(i)).num_cpus;
        VMId_t vm = VM_Create(LINUX, Machine_GetInfo(machines[i]).cpu);
        vms.push_back(vm);
        machines_vms_map[machines[i]].push_back(vm);
        VM_Attach(vm, machines[i]);
        machines_tc[machines[i]] = 0;
        total_fail[machines[i]] = 0;
        fail_count[machines[i]] = 0;

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
    tasks_per_vm = tasks_per_vm / GetNumTasks() +1;
    for(int i =0; i < Machine_GetTotal(); ++i){
        avg_fail[machines[i]] = 1;

    }
}

void Scheduler::MigrationComplete(Time_t time, VMId_t vm_id) {
    // Update your data structure. The VM now can receive new tasks
    migrating[vm_id] = false;
    
    
}
static unsigned long long ss = 0;
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


    bool added = false;
    for(int i =0; i < Machine_GetTotal(); ++i){
        MachineInfo_t  m_info = Machine_GetInfo(machines[i]);
        unsigned workload = 1;
        unsigned utilization = m_info.active_tasks;
        if(m_info.memory_size - m_info.memory_used - t_info.required_memory - VM_MEMORY_OVERHEAD < 0 || 
            workload + utilization > avg_fail[machines[i]] ){ 
            continue;
        }
        if(!added){
            for(int j = 0; j < machines_vms_map[machines[i]].size(); ++j){
                VMId_t vm = machines_vms_map[machines[i]][j];
                VMInfo_t v_info = VM_GetInfo(vm);
                if(migrating[vm]){
                    continue;
                }
                if( (v_info.active_tasks.size() > tasks_per_vm) ){

                    break;//add a new vm
                }
                if(!(v_info.vm_type == t_info.required_vm && t_info.required_cpu == v_info.cpu)){

                    continue;
                }
                added = true;
                //add task to vm
                AddTask(task_id, vm, priority);
                break;
            }
        }
        if(!added){
            //create_vm
            VMId_t vm = VM_Create( t_info.required_vm, t_info.required_cpu);
            VM_Attach(vm, m_info.machine_id);
            machines_vms_map[m_info.machine_id].push_back(vm);
            vms.push_back(vm);
            added = true;
            AddTask(task_id, vm, priority);
        }

    }
    if(!added){
        //choose random machine
        int randomNum = std::rand() % machines.size();
        MachineInfo_t m_info = Machine_GetInfo(machines[randomNum]);
        //create_vm
        VMId_t vm = VM_Create( t_info.required_vm, t_info.required_cpu);
        VM_Attach(vm, m_info.machine_id);
        machines_vms_map[m_info.machine_id].push_back(vm);
        vms.push_back(vm);
        added = true;
        AddTask(task_id, vm, priority);
    }

}
void Scheduler::AddTask(TaskId_t task_id, VMId_t vm_id, Priority_t priority) {
    VMInfo_t v_info = VM_GetInfo(vm_id);
    TaskInfo_t t_info = GetTaskInfo(task_id);
    VM_AddTask(vm_id, task_id, priority);
    tasks[task_id] = vm_id;
    // cout<< t_info.total_instructions << endl;
    machines_tc[v_info.machine_id]++;
    machines_mm[v_info.machine_id]++;
}
void Scheduler::RemoveTask(TaskId_t task_id, VMId_t vm_id) {
    VMInfo_t v_info = VM_GetInfo(vm_id);
    TaskInfo_t t_info = GetTaskInfo(task_id);
    tasks.erase(task_id);
    
    machines_tc[v_info.machine_id]--;
    
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
    // cout<< instr << " " << Now() << endl;
    SimOutput("SimulationComplete(): Finished!", 4);
    SimOutput("SimulationComplete(): Time is " + to_string(time), 4);
}

void Scheduler::TaskComplete(Time_t now, TaskId_t task_id) {
    // Do any bookkeeping necessary for the data structures
    // Decide if a machine is to be turned off, slowed down, or VMs to be migrated according to your policy
    // This is an opportunity to make any adjustments to optimize performance/energy

    VMId_t vm = tasks[task_id];
    RemoveTask(task_id, vm);
    MachineId_t machine = VM_GetInfo(vm).machine_id;
    avg_fail[machine]++;
    //Hcat gpt code to sort vector by respective value in map.
    // Sort vector based on map values
    std::sort(machines.begin(), machines.end(), [this](int a, int b) {
        return Machine_GetInfo(a).active_tasks*1.0 / avg_fail[a] > Machine_GetInfo(b).active_tasks*1.0 / avg_fail[b];
    });
    if(VM_GetInfo(vm).active_tasks.size() == 0 && !migrating[vm]){
        VM_Shutdown(vm);
        machines_vms_map[machine].erase(std::remove(machines_vms_map[machine].begin(), machines_vms_map[machine].end(), vm), machines_vms_map[machine].end());
        vms.erase(std::remove(vms.begin(), vms.end(), vm), vms.end());
    }

    SimOutput("Scheduler::TaskComplete(): Task " + to_string(task_id) + " is complete at " + to_string(now), 1);
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
    Scheduler.TaskComplete(time, task_id);
    SimOutput("HandleTaskCompletion(): Task " + to_string(task_id) + " completed at time " + to_string(time), 4);
}

void MemoryWarning(Time_t time, MachineId_t machine_id) {
    // The simulator is alerting you that machine identified by machine_id is overcommitted
    SimOutput("MemoryWarning(): Overflow at " + to_string(machine_id) + " was detected at time " + to_string(time), 1);
}

void MigrationDone(Time_t time, VMId_t vm_id) {
    // The function is called on to alert you that migration is complete
    SimOutput("MigrationDone(): Migration of VM " + to_string(vm_id) + " was completed at time " + to_string(time), 4);
    Scheduler.MigrationComplete(time, vm_id);
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
    for (const auto& pair : Scheduler.machines_mm) {
        std::cout << "Machine ID: " << pair.first << " -> Task Count: " << pair.second << std::endl;
    }

    Scheduler.Shutdown(time);
}

void SLAWarning(Time_t time, TaskId_t task_id) {
    for (const auto& pair : Scheduler.machines_vms_map) {
        std::cout << "Machine ID: " << pair.first << " -> Number of VMs: " << pair.second.size() << std::endl;
    }
    
    VMId_t vm = Scheduler.tasks[task_id];
    MachineId_t machine = VM_GetInfo(vm).machine_id;
    Scheduler.fail_count[machine]++;
    Scheduler.total_fail[machine] += Machine_GetInfo(machine).active_tasks;
    Scheduler.avg_fail[machine] = Scheduler.total_fail[machine]/Scheduler.fail_count[machine];
    SimOutput("num tasks " + to_string(Machine_GetInfo(machine).active_tasks) + " vms " + to_string(Machine_GetInfo(machine).active_vms)+ " fail " + to_string(Scheduler.total_fail[machine]) + " fail " + to_string(Scheduler.fail_count[machine])+ " fail " + to_string(Scheduler.avg_fail[machine]) , 0);

    //migrate excess tasks
    std::sort(Scheduler.machines.begin(), Scheduler.machines.end(), [](int a, int b) {
        return Scheduler.machines_vms_map[a].size() < Scheduler.machines_vms_map[b].size();
    });
    cout<<" mv: " << Scheduler.machines_vms_map[machine].size()/2  << "   tot " << Scheduler.vms.size() << " "<< Scheduler.machines[0]<< endl;
    int size = Scheduler.machines_vms_map[machine].size();
    for(int i = Scheduler.machines_vms_map[machine].size() -1; i > size/2; --i){
        
        VMId_t vm2 = Scheduler.machines_vms_map[machine][i];

        if(Scheduler.migrating[vm2]){
            continue;
        }
        for( auto task :VM_GetInfo(vm2).active_tasks){
            SetTaskPriority(task, HIGH_PRIORITY);
        }
        Scheduler.machines_tc[machine]-= VM_GetInfo(vm2).active_tasks.size();
        VM_Migrate(vm2, Scheduler.machines[0]);
        Scheduler.migrating[vm2] = true;
        Scheduler.machines_vms_map[machine].erase(std::remove(Scheduler.machines_vms_map[machine].begin(), Scheduler.machines_vms_map[machine].end(), vm2), Scheduler.machines_vms_map[machine].end());
        Scheduler.machines_vms_map[Scheduler.machines[0]].push_back(vm2);

        if(VM_GetInfo(Scheduler.machines_vms_map[machine][i]).active_tasks.size() + Machine_GetInfo(Scheduler.machines[0]).active_tasks > Scheduler.avg_fail[Scheduler.machines[0]] ){ 
            std::sort(Scheduler.machines.begin(), Scheduler.machines.end(), [](int a, int b) {
                return Scheduler.machines_vms_map[a].size() < Scheduler.machines_vms_map[b].size();
            });
        }

    }

    // SimOutput("actual time " + to_string(time) + " expected " + to_string(GetTaskInfo(task_id).target_completion) + "  arrival "+ to_string(GetTaskInfo(task_id).arrival), 0);
}

void StateChangeComplete(Time_t time, MachineId_t machine_id) {
    // Called in response to an earlier request to change the state of a machine
}
