/*Non-Canonical Input Processing*/
#include "linkLayer.h"

int send_control_package(int fd, unsigned char control, int file_size, unsigned char* file_name)
{
	int i, L1 = 1;
	unsigned char byte;
	int temp = file_size / 256;
	while(temp > 0)
	{
	temp = temp / 256;
	L1++;
	}
	int L2 = strlen(file_name);
	unsigned char package[L1+L2+5];
	package[0] = control;
	package[1] = 0x00;
	package[2] = (unsigned char) L1;
	temp = file_size / 256;
	for(i = 1; i <= L1; i++)
	{
		if(i == L1)
			package[2+i] = (unsigned char) file_size % 256;
		else
		{
			byte = (temp % 256);
			package[2+i] = byte;
			temp = temp / 256;
		}
	}
	package[L1+3] = 0x01;
	package[L1+4] = (unsigned char) L2;
	for(i = 1; i <= L2; i++)
	{
		package[L1+4+i] = file_name[i-1];
	}
	int ret = llwrite(fd, package, L1+L2+5);
	return ret;
}

int send_data_package(int fd, int seq_number, int num_bytes, unsigned char* buf)
{
	printf("data package\n");
	int i;
	unsigned char package[num_bytes+4];
	package[0] = 0x01;
	package[1] = (unsigned char) seq_number;
	package[2] = (unsigned char) num_bytes / 256;
	package[3] = (unsigned char) num_bytes % 256;
	for(i = 0; i < num_bytes; i++)
	{
		package[4+i] = buf[i];
	}
	int ret = llwrite(fd, package, num_bytes+4);
	return ret;
}

//MAIN FUNCTION
int main(int argc, char** argv)
{

	if(strcmp(argv[2], "receiver") == 0) // Receiver
	{
		status = RECEIVER;
		int fd = llopen(atoi(argv[1]), RECEIVER);
		int res = 0, i = 4, numDataPackets = 0, dataSeqNum = numDataPackets % 255;
		unsigned char* buffer;
		buffer = (unsigned char*) malloc(sizeof(unsigned char)*(MAX_PACKET_SIZE+4));
		do
		{
			res = llread(fd, buffer);
		}while(buffer[0] != C_START);
		int L2_index = buffer[2]+4;
		int name_length = buffer[L2_index];
		char* new_file_name = (char*) malloc(sizeof(char)*name_length);
		for(i = 0; i < name_length; i++)
		{
			new_file_name[i] = buffer[L2_index+1+i];
		}
		FILE* final_file = fopen(new_file_name, "w");
		unsigned char temp[1];
		do
		{
			res = llread(fd, buffer);
			if(res > 0 && buffer[0] == PACKET_DATA && dataSeqNum == buffer[1])
			{
				numDataPackets++;
				dataSeqNum = numDataPackets % 255;
				printf("Data:\n");
				for(i=4; i < res; i++)
				{
					printf("%02x  ", buffer[i]);
					temp[0] = buffer[i];
					fwrite(temp, sizeof(char), sizeof(temp), final_file);
				}
				printf("\n\n");
			}
		}while(buffer[0] != C_END);
		int file_size = -1, expected_size = 0;
		fseek(final_file , 0, SEEK_END);
		file_size = ftell(final_file );
		if(buffer[1] == 0x00)
		{
			int numBytes = buffer[2];
			for(i = 3; i < numBytes+3; i++)
			{
				expected_size = expected_size * 256;
				expected_size += buffer[i];
			}
		}
		if(expected_size == file_size)
			printf("Transmitted file size is correct.\n");
		free(buffer);
		fclose(final_file);
		llclose(fd);
	}
	else if(strcmp(argv[2], "transmitter") == 0) // Transmitter
	{

		unsigned char* file_name = argv[3];
		status = TRANSMITTER;
		FILE * f = fopen(file_name, "r");

		if(f <= 0) {
			printf("Error opening file\n");
			return -1;
		}

		//getting file size
		int file_size = -1;
		fseek(f, 0, SEEK_END);
		file_size = ftell(f);
		fseek(f, 0, SEEK_SET);

		int fd = llopen(atoi(argv[1]), TRANSMITTER);
		if(fd < 0) {
			printf("> Error on serial port connection\n");
			return -1;
		}


		// start control packet
		printf("Send start control package\n");
		if(send_control_package(fd, PACKET_START, file_size, file_name) < 0)
		{

			fclose(f);
			llclose(fd);
			return -1;
		}

		int sequence_nr = 0;

		// Send data packets rever nomeas cenas 
		int nr_packets = (file_size / MAX_PACKET_SIZE) + (file_size % MAX_PACKET_SIZE > 0 ? 1 : 0);
		unsigned char buf[MAX_PACKET_SIZE];
		int i;
		for(i = 0; i < nr_packets; i++)
		{
			printf("Sending data package number %d...\n", (i+1));
			int num_bytes = fread(buf, sizeof(unsigned char), MAX_PACKET_SIZE, f);
			if(num_bytes == 0)
				if(ferror(f) != 0)
					printf("Error\n");
				else if(feof(f) != 0)
					printf("EOF\n");
			printf("%d bytes read from file.\n", num_bytes);
			int n = sequence_nr % 255;
			if(send_data_package(fd, n, num_bytes, buf) < 0)
			{
				printf("Error sending package number %d\n", i+1);
				fclose(f);
				llclose(fd);
				return -1;
			}
			sequence_nr += 1;
		}

		// End control packet
		printf("Send end control package\n");
		send_control_package(fd, PACKET_END,  file_size, file_name);
		
		if(fclose(f) < 0) {
			printf("Error closing the input file\n");
			llclose(fd);
			return -1;
		}

		llclose(fd);
	}

	return 0;
}
