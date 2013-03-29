#include "kernel.h"

#include <iostream>
#include <string>
#include <fstream>
#include <algorithm>
#include <limits>

namespace vm
{
	bool cmpPrior(Process first, Process second)
	{		
		return first.priority > second.priority;
	}
    Kernel::Kernel(Scheduler scheduler, std::vector<std::string> executables_paths)
        : machine(), processes(), priorities(), scheduler(scheduler),
          _last_issued_process_id(0),
          _last_ram_position(0),
          _current_process_index(0),
          _cycles_passed_after_preemption(0)
    {
        std::for_each(executables_paths.begin(), executables_paths.end(), [&](const std::string &path) {
            CreateProcess(path);
        });
		int counter=0;
		int cycles= 0;
        if (scheduler == FirstComeFirstServed || scheduler == ShortestJob) {
            machine.pic.isr_0 = [&]() {
				
				std::cout << "Kernel: processing the timer interrupt"<< std::endl;
				std::cout << "Kernel: allowing the current proccess to run "<<counter<< std::endl;
				std::cout << "The current cycle is : " << cycles << std::endl;
				std::cout<<" " <<std::endl;
				++cycles;
				// ToDo: Process the timer interrupt for the FCFS or Shortest Job scheduler
			};
			
            machine.pic.isr_3 = [&]() {
				cycles=0;
				std::cout << "Kernel: processing the software interrupt"<< std::endl;
				std::cout << "Kernel: unloading the process "<<counter<< std::endl;
				
				processes.pop_front();
				++counter;
				if(!processes.empty())
				{
					std::cout << "Kernel: switching the context from process "<<counter-1<<" to process "<<counter<< std::endl;
					std::cout<<" "<<std::endl;
					machine.cpu.registers = processes.front().registers;
				}	
				
				else
				{
					machine.Stop();
				}
                // ToDo: Process the first software interrupt for the FCFS or Shortest Job scheduler
				// Unload the current process
            };
        } else if (scheduler == RoundRobin) {
			
            machine.pic.isr_0 = [&]() {
				
				if( _cycles_passed_after_preemption >= Kernel::_MAX_CYCLES_BEFORE_PREEMPTION)
				{
					std::cout << "Kernel: processing the timer interrupt"<< std::endl;
					Process current_proccess  = processes.front();
					current_proccess.registers= machine.cpu.registers;
					int current_id = processes.front().id;
					processes.pop_front();
					processes.push_back(current_proccess);
					if(!processes.empty())
					{
							
						std::cout << "Kernel: switching the context from process "<<current_id<<" to process "<< processes.front().id << std::endl;
						std::cout<<" " <<std::endl;
						machine.cpu.registers=processes.front().registers;
						_cycles_passed_after_preemption=0;
					}
					
					
				}
				else
				{
					std::cout << "Kernel: processing the timer interrupt"<< std::endl;
					std::cout << "Kernel: allowing the current proccess to run "<< processes.front().id<< std::endl;
					std::cout << "The current cycle is : " << _cycles_passed_after_preemption << std::endl;
					std::cout<<" " <<std::endl;
					++_cycles_passed_after_preemption;
				}



				// ToDo: Process the timer interrupt for the Round Robin scheduler
			};

            machine.pic.isr_3 = [&]() {

				if(!processes.empty())
				{
					std::cout << "Kernel: processing the software interrupt"<< std::endl;
					std::cout << "Kernel: unloading the process "<< processes.front().id<< std::endl;
					processes.pop_front();		
					if(!processes.empty())
					{
						std::cout << "Kernel: switching the context to process "<<processes.front().id<< std::endl;
						std::cout<<" " <<std::endl;
						machine.cpu.registers = processes.front().registers;
					}
					else 
					{
						machine.Stop();
					}
				}
				else
				{
					machine.Stop();
				}

				

                // ToDo: Process the first software interrupt for the Round Robin scheduler
				// Unload the current process
            };
        } else if (scheduler == Priority) {
            machine.pic.isr_0 = [&]() {
				if(!processes.empty())
				{
					std::cout << "Kernel: processing the timer interrupt"<< std::endl;
					std::cout << "Kernel: allowing the current proccess to run "<< processes.front().id<< std::endl;
					std::cout << "The current cycle is : " << _cycles_passed_after_preemption << std::endl;
					std::cout<<" " <<std::endl;
					++_cycles_passed_after_preemption;
					int current_id = processes.front().id;
					--processes.front().priority;
					processes.front().registers = machine.cpu.registers;
					std::sort(processes.begin(), processes.end(),cmpPrior);
					if(current_id != processes.front().id)
					{
						std::cout << "Kernel: switching the context to process "<<processes.front().id<< std::endl;
						_cycles_passed_after_preemption=0;
					}
					machine.cpu.registers = processes.front().registers;
					
				}
			};

            machine.pic.isr_3 = [&]() {
					if(!processes.empty())
				{
					std::cout << "Kernel: processing the software interrupt"<< std::endl;
					std::cout << "Kernel: unloading the process "<< processes.front().id<< std::endl;
					processes.pop_front();		
					if(!processes.empty())
					{
						std::cout << "Kernel: switching the context to process "<<processes.front().id<< std::endl;
						std::cout<<" " <<std::endl;
						machine.cpu.registers = processes.front().registers;
					}
					else 
					{
						machine.Stop();
					}
				}
				else
				{
					machine.Stop();
				}
			
                // ToDo: Process the first software interrupt for the Priority Queue scheduler
				// Unload the current process
            };
        }

        machine.Start();
    }
	
	

    Kernel::~Kernel() {}
	bool cmpJob(Process first, Process second)
	{
		return first.sequential_instruction_count < second.sequential_instruction_count;
	}
	
    void Kernel::CreateProcess(const std::string &name)
    {
        if (_last_issued_process_id == std::numeric_limits<Process::process_id_type>::max()) {
            std::cerr << "Kernel: failed to create a new process. The maximum number of processes has been reached." << std::endl;
        } else {
            std::ifstream input_stream(name, std::ios::in | std::ios::binary);
            if (!input_stream) {
                std::cerr << "Kernel: failed to open the program file." << std::endl;
            } else {
                Memory::ram_type ops;

                input_stream.seekg(0, std::ios::end);
                auto file_size = input_stream.tellg();
                input_stream.seekg(0, std::ios::beg);
                ops.resize(static_cast<Memory::ram_size_type>(file_size) / 4);

                input_stream.read(reinterpret_cast<char *>(&ops[0]), file_size);

                if (input_stream.bad()) {
                    std::cerr << "Kernel: failed to read the program file." << std::endl;
                } else {
                    std::copy(ops.begin(), ops.end(), (machine.memory.ram.begin() + _last_ram_position));

                    Process process(_last_issued_process_id++, _last_ram_position,
                                                               _last_ram_position + ops.size());

                    _last_ram_position += ops.size();

					if( scheduler == ShortestJob)
					{
						std::sort(processes.begin(), processes.end(), cmpJob);
					}
					else if(scheduler == Priority)
					{
						process.priority = rand() % 100+1;
						std::sort(processes.begin(), processes.end(),cmpPrior);
					}
                    // ToDo: add the new process to an appropriate data structure
                    processes.push_back(process);
					
                    // ToDo: process the data structure
                }
            }
        }
    }
}
