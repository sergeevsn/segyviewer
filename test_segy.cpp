#include "SegyReader.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <segy_file>" << std::endl;
        return 1;
    }
    
    try {
        std::cout << "Testing SegyReader with file: " << argv[1] << std::endl;
        SegyReader reader(argv[1]);
        
        std::cout << "Successfully opened SEG-Y file!" << std::endl;
        std::cout << "Number of traces: " << reader.num_traces() << std::endl;
        std::cout << "Samples per trace: " << reader.num_samples() << std::endl;
        std::cout << "Sample interval: " << reader.sample_interval() << " ms" << std::endl;
        
        // Попробуем прочитать первую трассу
        if (reader.num_traces() > 0) {
            auto trace = reader.get_trace(0);
            std::cout << "First trace loaded with " << trace.size() << " samples" << std::endl;
            std::cout << "First 5 samples: ";
            for (int i = 0; i < 5 && i < trace.size(); ++i) {
                std::cout << trace[i] << " ";
            }
            std::cout << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
