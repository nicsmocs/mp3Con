#include <filesystem>
#include <iostream>
#include "nicsmocs.h"
#include "lame.h"

std::string inputSuffix = ".wav";

std::mutex m;

bool keepGoing = true;

int getWavSampleRate(FILE* pcm)
{
	uint8_t buffer[28];
	if (!fread(buffer, 28, 1, pcm))
	{
		return 0;
	}

	uint32_t rate = 0;
	memcpy(&rate, buffer + 24, 4);
	return rate;
}

int getNCores()
{
	return std::thread::hardware_concurrency();
}


void convertToMp3(const std::string& fileName, const std::string& folder)
{
	int read, write;
	FILE* pcm = fopen((fileName).c_str(), "rb");
	if (!pcm) 
	{
		std::cerr << "Error opening " << fileName << std::endl;
		exit(1);
	}
	fseek(pcm, 4 * 1024, SEEK_CUR);
	rewind(pcm);

	const auto fileNameWithoutSuffix = fileName.substr(0, fileName.size() - inputSuffix.size());

	auto mp3 = fopen((fileNameWithoutSuffix + ".mp3").c_str(), "wb");
	if (!mp3)
	{
		std::cerr << "Error opening " << fileNameWithoutSuffix << ".mp3 for writing" << std::endl;
		exit(3);
	}

	const int PCM_SIZE = 8192 * 3;
	const int MP3_SIZE = 8192 * 3;
	short int pcm_buffer[PCM_SIZE * 2];
	unsigned char mp3_buffer[MP3_SIZE];

	lame_t lame = lame_init();
	lame_set_in_samplerate(lame, getWavSampleRate(pcm));
	lame_set_VBR(lame, vbr_default);
	lame_init_params(lame);

	int nTotalRead = 0;

	do
	{
		read = fread(pcm_buffer, 2 * sizeof(short int), PCM_SIZE, pcm);
		nTotalRead += read * 4;
		if (read == 0)
		{
			write = lame_encode_flush(lame, mp3_buffer, MP3_SIZE);
		}
		else
		{
			write = lame_encode_buffer_interleaved(lame, pcm_buffer, read, mp3_buffer, MP3_SIZE);
		}

		fwrite(mp3_buffer, write, 1, mp3);
	} while (read != 0);

	lame_close(lame);
	fclose(mp3);
	fclose(pcm);
}

void process(std::vector<std::string>* fileNames, const std::string& outputPath, int tId)
{
	for (;;)
	{
		std::unique_lock<std::mutex> guard(m);
		if (fileNames->size() == 0) 
		{
			break;
		}
		const auto target = fileNames->back();
		fileNames->pop_back();
		guard.unlock();
		std::cout << "Started converting " << target << " (t" << tId << ")" << std::endl;
		convertToMp3(target, outputPath);
		std::cout << "Done converting " << target << " (t" << tId << ")" << std::endl;
	}
}

std::vector<std::string> getInputFiles(const std::string& path)
{
	std::vector<std::string> result;
	std::cout << "filenames to convert:" << std::endl;
	for (const auto& entry : std::filesystem::recursive_directory_iterator(path))
	{
		if (entry.is_directory())
		{
			continue;
		}

		try
		{
			auto file = entry.path().string();
			if (entry.path().extension().generic_string() == ".wav")
			{
				std::cout << file << std::endl;
				result.push_back(file);
			}
		}
		catch (...)
		{
			std::cout << "ERROR: problem with the filename." << std::endl;
			return result;
		}
	}
	return result;
}

void printHelp()
{
	std::cout << "Mp3Con converts your wav files to mp3s. The mp3s are copied next to your wavs with the same name.\nGive a path as 1st argument for a location of the wavs you want to convert. If no argument given then the program will search in the current location. The search happens recursively in all subfolders." << std::endl << std::endl;
}

int main(int argc, char* argv[])
{
	Init();
	printHelp();
	std::string path = "";
	if (argc < 2)
	{
		path = ".";
	}
	else
	{
		path = argv[1];
	}

	auto fileNames = getInputFiles(path);
	if (fileNames.empty())
	{
		std::cout << "no .wav files found" << std::endl;
		return -1;
	}

	const auto nThreads = getNCores();
	std::vector<std::thread> threads;

	for (auto i = 0; i < nThreads; i++) 
	{
		threads.push_back(std::thread(process, &fileNames, path, i));
	}

	for (auto i = 0; i < nThreads; i++) 
	{
		threads[i].join();
	}

	return 0;
}