#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>

#define FNAME		"pi_hex_1b.txt" // https://archive.org/details/pi_hex_1b
#define PATTERN		(uint64_t)0xBD3E5E800000000
#define MASK		(uint64_t)0xFFFFFF800000000
#define BUFFER_SIZE	(10 * 1024)

static int hex_to_uint32(uint32_t *dest, const char *buffer, size_t buff_size)
{
	while(buff_size > 8)
	{
		char c;
		*dest = 0;
		for(int i = 0; i < 7; i++)
		{
			c = *buffer++;
			*dest += (c < 'a') ? (c - '0') : (c - 'a' + '9' - '0' + 1);
			*dest = *dest << 4;
		}

		c = *buffer++;
		*dest++ += (c < 'a') ? (c - '0') : (c - 'a' + '9' - '0' + 1);
		buff_size -= 8;
	}

	return 0;
}

int read_part(uint32_t *var, ssize_t *left, FILE *fp)
{
	static uint32_t data[BUFFER_SIZE/8];
	static size_t data_left;
	static size_t data_loaded;
	static ssize_t bytes_left;
	static int dot;

	if(data_left == 0)
	{
		size_t read_bytes;
		char buffer[BUFFER_SIZE];
		if(!(read_bytes = fread(buffer, 1, sizeof(buffer), fp)))
			return -1;

		if(dot == 0)
		{
			char *dot2 = strchr(buffer, '.');

			ptrdiff_t dot_offset = dot2 - buffer;
			dot_offset = read_bytes - dot_offset;
			fseek(fp, -dot_offset+1, SEEK_CUR);
			read_bytes = fread(buffer + read_bytes - dot_offset, 1, dot_offset, fp) - dot_offset + read_bytes;
			dot = 1;
		}

		hex_to_uint32(data, buffer, read_bytes);

		bytes_left = (read_bytes % 2) ? (read_bytes/2)+1 : read_bytes/2;
		data_left = (read_bytes % 8) ? (read_bytes/8)+1 : read_bytes/8;
		data_loaded = data_left;
	}

	*var = data[ data_loaded - (data_left--) ];
	*left = (bytes_left -= 4);

	return 0;
}

int find_pattern(FILE *fp)
{
	unsigned long bit_pos = 0;
	ssize_t bytes_left;
	int ret = 0;

	uint64_t var;
	// Init high 32 bits
	if(read_part((uint32_t *)&var, &bytes_left, fp))
	{
		fprintf(stderr, "Failed to init var\n");
		return -1;
	}
	var = var << 32;

	while(!read_part((uint32_t *)&var, &bytes_left, fp))
	{
		for(int i = 0; i < 32; i++)
		{
			if((var & MASK) == PATTERN)
			{
				if(bytes_left < 0 && i/8 - bytes_left < 4)
					break;

				ret++;
				printf("Found pattern at bit %lu\n", bit_pos);
			}

			bit_pos++;
			var = var << 1;
		}
	}

	return ret;
}

int main()
{
	FILE *fp = fopen(FNAME, "r");
	if(!fp)
	{
		fprintf(stderr, "Failed to read file\n");
		return -1;
	}

	int count = find_pattern(fp);
	printf("Found %d patterns\n", count);

	fclose(fp);

	return 0;
}
