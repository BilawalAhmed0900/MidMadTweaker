#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <exception>
#include <cstdint>

enum class LogLevel 
{
	ERROR = 1 << 8, WARN, INFO
};

class Log
{
private:
	LogLevel log_level;
	void info(std::string &message)
	{
		if (log_level >= LogLevel::INFO)
		{
			std::cout << "[INFO] " << message << std::endl;
		}
	}

	void warn(std::string &message)
	{
		if (log_level >= LogLevel::WARN)
		{
			std::cout << "[WARNING] " << message << std::endl;
		}
	}

	void error(std::string &message)
	{
		if (log_level >= LogLevel::ERROR)
		{
			std::cout << "[ERROR] " << message << std::endl;
		}
	}

public:
	void set_level(LogLevel lvl)
	{
		this->log_level = lvl;
	}

	LogLevel get_level()
	{
		return this->log_level;
	}

	void log(std::string message, LogLevel lvl = LogLevel::ERROR)
	{
		if (lvl == LogLevel::INFO)
		{
			this->info(message);
		}
		else if (lvl == LogLevel::WARN)
		{
			this->warn(message);
		}
		// LogLevel::ERROR
		else
		{
			this->error(message);
		}
	}

	Log() : log_level(LogLevel::ERROR) { }
	Log(LogLevel lvl) { this->log_level = lvl; }
	~Log() { }
};

// Global log class
Log log(LogLevel::INFO);

void print_help()
{
	std::cout << 
		"Midtown Madness Car Unlocker made by Dragneel1234@Github\n"
		"Usage: MidMadTweaker.exe [-h] Input Output\n"
		"\n"
		"Positional Arguments:\n"
		"    Input: Input file to read from\n"
		"    Output: Output file to write to\n"
		"\n"
		"Optional Arguments:\n"
		"    -h: Show this help and exit\n"
		"\n"
		"Note: Input and Output can both be same...\n";
	std::cout.flush();
}

bool file_exists(std::string file_name)
{
	std::ifstream file(file_name);
	if (file.is_open())
	{
		file.close();
		return true;
	}

	return false;
}

int32_t count_till(char *str, char chr)
{
	int32_t ret = 0;
	while (*str++ != chr)
		ret++;

	return ret;
}

void set_zero_till_next_line(char *str)
{
	const uint8_t LINE_DELIM = 0x0D;
	while (*str != LINE_DELIM)
	{
        /*
            '0' not '\0'
            We have to set Unlock Score and Flags to '0' not '\0'
        */
		*str = '0';
		str++;
	}
}

void copy_till_next_line(char *dst, char *src)
{
	const uint8_t LINE_DELIM = 0x0D;
	while (*src != LINE_DELIM)
	{
		*dst = *src;
		src++;
		dst++;
	}
}

// Midtown Madness UI file
class MM_UI
{
private:
	char *buffer;
	uint32_t size;

public:
	MM_UI() : buffer(NULL) { }

	MM_UI(std::string file_name)
	{
		log.log("Opening input file...", LogLevel::INFO);
		std::ifstream stream(file_name, std::ios_base::binary);
		if (stream.is_open() == false)
		{
			throw std::runtime_error("Can\'t open input file");
		}

		log.log("Getting file size...", LogLevel::INFO);
		stream.seekg(0, std::ios_base::end);
		this->size = stream.tellg();
		stream.seekg(0, std::ios_base::beg);

		log.log("Storing file data in buffer...", LogLevel::INFO);
		this->buffer = new (std::nothrow) char[this->size];
		if (this->buffer == NULL)
		{
			char message[1<<8];
			sprintf(message, "Can\'t allocate %u bytes", this->size);
			throw std::runtime_error(message);
		}
		stream.read(this->buffer, this->size);

		log.log("Verifying buffer...", LogLevel::INFO);

		/*
			ARES at 0 and BaseName at 0x12F7B60
		 	Car info structures start at that offset
		 	BaseName is the first member to give an ID to car
		*/
		if (std::memcmp(this->buffer, "ARES", 4) != 0 || 
			std::memcmp(&this->buffer[0x12F7B60], "BaseName", 8) != 0)
		{
			throw std::runtime_error("Input file is an invalid UI.ar file");
		}

		log.log("Closing input file...", LogLevel::INFO);
		stream.close();
	}
    
private: // Private one, used by unlock_all
    void unlock_one(char *car_data)
	{
		const int32_t CAR_NAME_BUFFER_SIZE = 32;
		char car_name[CAR_NAME_BUFFER_SIZE];
		std::memset(car_name, 0, CAR_NAME_BUFFER_SIZE);
		copy_till_next_line(car_name, car_data);

		char message[1024];
		sprintf(message, "Unlocking car %s", car_name + count_till(car_data, '=') + 1);
		log.log(message, LogLevel::INFO);

		int32_t total_stepped = 0;
		for (int32_t loop_counter = 0; loop_counter < 2; loop_counter++)
		{
			/*
				A car structure has 12 entries
				example:
					BaseName=vpbus
					Description=City Bus
					Colors=White|Blue|Orange|Red
					Flags=16
					Order=-1
					ScoringBias=100.0
					UnlockScore=0
					UnlockFlags=32
					Horsepower=400	
					Top Speed=91 	
					Durability=2000000
					Mass=8310

				We have to set UnlockScore and UnlockFlags to 0, to have it unlocked by default
				Others are not necessary but you can play with them

				Each struct ends with \0 so, treated as single string
			*/
			
			// First U for UnlockScore, than for UnlockFlag
			int32_t count = count_till(car_data, 'U');
			total_stepped += count;
			car_data += count;

			// Finding '='
			count = count_till(car_data, '=');
			total_stepped += count;
			car_data += count;

			// Get ahead of '=' to actual value
			car_data++;
			total_stepped++;

			// Set values to '0' till 0x0D, indicator for next line
			set_zero_till_next_line(car_data);
		}
	}

public:
	void unlock_all()
	{
		/*
			There are total ten cars
			Each car has its own structure
			Structures are padded to nearest multiple of 16
		*/
		const int32_t cars_count = 10;
		const int32_t structure_sizes[cars_count] = 
			{0xD0, 0xD0, 0xE0, 0xD0, 0xD0, 0xE0, 0xE0, 0xF0, 0xD0, 0xE0};

		// Car info structures start at that offset
		char *car_data = &this->buffer[0x12F7B60];
		for (int32_t loop_counter = 0; loop_counter < cars_count; loop_counter++)
		{
			unlock_one(car_data);
			car_data += structure_sizes[loop_counter];
		}
	}

	void save_to_file(std::string filename)
	{
		log.log("Saving to output file...", LogLevel::INFO);
		std::ofstream stream(filename, std::ios_base::binary);
		if (stream.is_open() == false)
		{
			throw std::runtime_error("Can\'t create output file");
		}

		stream.write(this->buffer, this->size);
		stream.close();
	}

	~MM_UI()
	{
		if (this->buffer != NULL)
		{
			delete[] this->buffer;
		}
	}
};

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
    	if (argc == 2 && std::strcmp(argv[1], "-h") == 0);
		else
		{
			log.log("Required at least one arguments...");			
		}
		print_help();

    	return EXIT_FAILURE;
    }

    std::string input(argv[1]), output(argv[2]);
    std::cout << "Midtown Madness Car Unlocker made by Dragneel1234@Github\n";
    try
    {
    	MM_UI mm_ui(input);
    	mm_ui.unlock_all();
    	mm_ui.save_to_file(output);
    }
    catch(std::runtime_error &e)
    {
    	log.log(e.what(), LogLevel::ERROR);
    	return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
