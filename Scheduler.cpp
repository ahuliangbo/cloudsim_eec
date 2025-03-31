//
//  Scheduler.cpp
//  CloudSim
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 10/20/24.
//

#include "Scheduler.hpp"

static bool migrating = false;
static unsigned active_machines = 16;

static unsigned long long Q1_avg = 0;
static unsigned long long Q2_avg = 0;
static unsigned long long Q3_avg = 0;
static unsigned long long Q4_avg = 0;

static int Q1 = 0;
static int Q2 = 0;
static int Q3 = 0;

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
        machines_map[machines[i]] = 0;
        machines_mm[machines[i]] = 0;
        VMId_t vm = VM_Create(LINUX, Machine_GetInfo(machines[i]).cpu);
        // vms.push_back(vm);
        machines_vms_map[machines[i]].push_back(vm);
        VM_Attach(vm, machines[i]);
      
    }
    std::sort(machines.begin(), machines.end(), [](MachineId_t a, MachineId_t b) {
        return Machine_GetInfo(a).performance[0] * Machine_GetInfo(a).num_cpus > Machine_GetInfo(b).performance[0] * Machine_GetInfo(b).num_cpus; 
    });

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

//chat gpt helper math functions
double calculateAverage(const std::vector<int>& nums) {
    if (nums.empty()) return 0.0;
    double sum = std::accumulate(nums.begin(), nums.end(), 0.0);
    return sum / nums.size();
}

void findQuartileAverages(std::vector<TaskId_t>& nums) {
    std::sort(nums.begin(), nums.end());
    int n = nums.size();
    int qSize = n / 4;

    // Divide into 4 quartiles
    std::vector<int> q1(nums.begin(), nums.begin() + qSize);
    std::vector<int> q2(nums.begin() + qSize, nums.begin() + 2 * qSize);
    std::vector<int> q3(nums.begin() + 2 * qSize, nums.begin() + 3 * qSize);
    std::vector<int> q4(nums.begin() + 3 * qSize, nums.end());

    // Calculate averages
    Q1_avg = calculateAverage(q1);
    Q2_avg= calculateAverage(q2);
    Q3_avg = calculateAverage(q3);
    Q4_avg = calculateAverage(q4);

    Q1 = qSize;
    Q2 = 2 * qSize;
    Q3 = 3 * qSize;
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
    tasks_vec.push_back(t_info.target_completion - t_info.arrival);
    findQuartileAverages(tasks_vec);

    int quarter = machines.size()/4;
    int start;
    int end;
    static int ss = 0;
    int average;
    instr += t_info.total_instructions ;
    cout<< Q1_avg<<" "<< Q2_avg<< " "<< Q3_avg<< " "<<Q4_avg<<" " << t_info.target_completion<< " ";
    //chatgpt comparator and maxHeap

    // std::cout << "Time: " << Now()<< " Target(): " << t_info.target_completion -t_info.arrival << " Instructions "<< avg_task_time  <<std::endl;
    //below average task count, give them least allocated machines.

    auto cmp = [](const std::pair<unsigned, unsigned long long>& a, const std::pair<unsigned, unsigned long long>& b) {
        return a.second > b.second; // Min-heap based on second element
    };
    if(t_info.target_completion - t_info.arrival < tasks_vec[Q1]){
        start = 0;
        end = quarter;
        average = Q1_avg;
        cout<< "1."<<endl;
    }else if (t_info.target_completion - t_info.arrival < tasks_vec[Q2]){
        start = quarter;
        end = quarter*2;
        average = Q2_avg;
        cout<< "2."<<endl;
    }
    else if (t_info.target_completion - t_info.arrival < tasks_vec[Q3]){
        start = quarter*2;
        end = quarter*3;
        average = Q3_avg;
        cout<< "3."<<endl;
    }else{
        start = quarter*3;
        end = Machine_GetTotal();
        average = Q4_avg;
        cout<< "4."<<endl;
    }
    std::priority_queue<std::pair<unsigned, unsigned long long>, std::vector<std::pair<unsigned, unsigned long long>>, decltype(cmp)> minHeap(cmp);
    for(int i =start; i < end; ++i){
        MachineInfo_t m_info = Machine_GetInfo(machines[i]);
        unsigned long long ETA = machines_map[machines[i]];
        minHeap.push({ machines[i],ETA });
    }
    for(int i =0; i < Machine_GetTotal(); ++i){
        MachineInfo_t m_info = Machine_GetInfo(minHeap.top().first);
        // cout << minHeap.top().first << " " << minHeap.top().second << endl;
        minHeap.pop();
        if(t_info.required_cpu != m_info.cpu){
            continue;
        }
        for(int i =0; i < machines_vms_map[m_info.machine_id].size(); ++i){
            VMInfo_t v_info = VM_GetInfo(machines_vms_map[m_info.machine_id][i]);
            if( t_info.required_cpu== v_info.cpu && t_info.required_vm == v_info.vm_type){
                if(t_info.target_completion < average){
                    priority = HIGH_PRIORITY;
                }
                AddTask(task_id, machines_vms_map[m_info.machine_id][i], priority);
                return;
            }
        }
        VMId_t vm = VM_Create( t_info.required_vm, t_info.required_cpu);
        VM_Attach(vm, m_info.machine_id);
        machines_vms_map[m_info.machine_id].push_back(vm);
        // vms.pushback();
        if(t_info.target_completion < average){
            priority = HIGH_PRIORITY;
        }
        AddTask(task_id, vm, priority);
        return;
        
    }   
}
void Scheduler::AddTask(TaskId_t task_id, VMId_t vm_id, Priority_t priority) {
    VMInfo_t v_info = VM_GetInfo(vm_id);
    TaskInfo_t t_info = GetTaskInfo(task_id);
    VM_AddTask(vm_id, task_id, priority);
    tasks[task_id] = vm_id;
    machines_map[v_info.machine_id] += t_info.target_completion-t_info.arrival;
    // cout<< t_info.total_instructions << endl;
}
void Scheduler::RemoveTask(TaskId_t task_id, VMId_t vm_id) {
    VMInfo_t v_info = VM_GetInfo(vm_id);
    TaskInfo_t t_info = GetTaskInfo(task_id);
    tasks.erase(task_id);
    machines_mm[v_info.machine_id]++;
    
    machines_map[v_info.machine_id] = machines_map[v_info.machine_id]<t_info.target_completion+t_info.arrival ?
    0 : machines_map[v_info.machine_id]-t_info.target_completion+t_info.arrival;
    
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
    cout<< instr << " " << Now() << endl;
    SimOutput("SimulationComplete(): Finished!", 4);
    SimOutput("SimulationComplete(): Time is " + to_string(time), 4);
}

void Scheduler::TaskComplete(Time_t now, TaskId_t task_id) {
    // Do any bookkeeping necessary for the data structures
    // Decide if a machine is to be turned off, slowed down, or VMs to be migrated according to your policy
    // This is an opportunity to make any adjustments to optimize performance/energy

    RemoveTask(task_id, tasks[task_id]);
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
    for (const auto& pair : Scheduler.machines_mm) {
        std::cout << "Machine ID: " << pair.first << " -> Task Count: " << pair.second << std::endl;
    }

    Scheduler.Shutdown(time);
}

void SLAWarning(Time_t time, TaskId_t task_id) {
    
    // SimOutput("actual time " + to_string(time) + " expected " + to_string(GetTaskInfo(task_id).target_completion) + "  arrival "+ to_string(GetTaskInfo(task_id).arrival), 0);
}

void StateChangeComplete(Time_t time, MachineId_t machine_id) {
    // Called in response to an earlier request to change the state of a machine
}

