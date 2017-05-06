#ifndef BITOPS_H
#define BITOPS_H

#define RBITS( c, n )		( c & ( 0xFF >> (8 - n) ) )
#define LBITS( c, n )		( c >> (8 - n) )
#define MBITS( c, l, r )	( RBITS( c,l ) >> r )
#define RBITS32( c, n )		( c & ( 0xFFFFFFFF >> (32 - n) ) )
#define MBITS32( c, l, r )	( RBITS32( c,l ) >> r )
#define BITN( c, n )		( (c >> n) & 0x1 )

#include <memory>
#include <vector>

enum StreamType {
	kFile = 0,
	kMemory = 1,
	kStream = 2
};

enum StreamMode {
	kRead = 0,
	kWrite = 1
};

	
/* -----------------------------------------------
	class to read arrays bitwise
	----------------------------------------------- */

class abitreader
{
public:
	abitreader( unsigned char* array, int size );
	~abitreader();	
	unsigned int read( int nbits );
	unsigned char read_bit();
	unsigned char unpad( unsigned char fillbit );
	int getpos();
	int getbitp();
	void setpos( int pbyte, int pbit );
	void rewind_bits( int nbits );
	bool eof();
	int peof();
	
private:
	unsigned char* data;
	int lbyte;
	int cbyte;
	int cbit;
	int peof_;
	bool eof_;
};


/* -----------------------------------------------
	class to write arrays bitwise
	----------------------------------------------- */

class abitwriter
{
public:
	abitwriter( int size );
	~abitwriter();	
	void write( unsigned int val, int nbits );
	void write_bit( unsigned char bit );
	void set_fillbit( unsigned char fillbit );
	void pad();
	unsigned char* getptr();
	int getpos();
	int getbitp();
	bool error();
	
private:
	unsigned char fillbit_;
	unsigned char* data;
	int dsize;
	int cbyte;
	int cbit;
	bool fmem;
	bool error_;
};


/* -----------------------------------------------
	class to read arrays bytewise
	----------------------------------------------- */

class abytereader
{
public:
	abytereader(const std::vector<std::uint8_t>& data);
	~abytereader();	
	int read( unsigned char* byte );
	int read_n( unsigned char* byte, int n );
	void seek( int pos );
	int getsize();
	int getpos();
	bool eof();
	
private:
	const std::vector<std::uint8_t> data_;
	std::vector<std::uint8_t>::const_iterator cbyte_; // The position in the data of the byte being read.
	bool eof_ = false;
};


/* -----------------------------------------------
	class to write arrays bytewise
	----------------------------------------------- */

class abytewriter
{
public:
	abytewriter();
	~abytewriter();	
	void write( unsigned char byte );
	void write_n(const unsigned char* byte, int n );
	unsigned char* getptr();
	int getpos();
	void reset();
	bool error();
	
private:
	std::vector<std::uint8_t> data_;
};


/* -----------------------------------------------
	class for input and output from file or memory
	----------------------------------------------- */

class iostream
{
public:
	iostream( void* src, StreamType srctype, int srcsize, StreamMode iomode );
	~iostream();
	void switch_mode();
	int read(unsigned char* to, int dtsize);
	bool read_byte(unsigned char* to);
	int write(const unsigned char* from, int dtsize);
	int write_byte(unsigned char byte);
	int flush();
	int rewind();
	int getpos();
	int getsize();
	unsigned char* getptr();
	bool chkerr();
	bool chkeof();
	
private:
	void open_file();
	void open_mem();
	void open_stream();
	
	int write_file(const unsigned char* from, int dtsize);
	int write_file_byte(unsigned char byte);
	int read_file(unsigned char* to, int dtsize);
	bool read_file_byte(unsigned char* to);
	int write_mem(const unsigned char* from, int dtsize );
	int write_mem_byte(unsigned char byte);
	int read_mem(unsigned char* to, int dtsize);
	bool read_mem_byte(unsigned char* to);
	
	FILE* fptr;
	std::vector<char> file_buffer; // Used to replace the default file buffer for reads/writes to improve performance.
  
	std::unique_ptr<abytewriter> mwrt;
	std::unique_ptr<abytereader> mrdr;
	
	bool free_mem_sw;
	void* source;
	StreamMode mode;
	StreamType srct;
	int srcs;
};

class ArithmeticBitWriter {
public:
	template <std::uint8_t bit>
	void write_bit() {
		// add bit at last position
		curr_byte_ = (curr_byte_ << 1) | bit;
		// increment bit position
		curr_bit_++;

		// write bit if done
		if (curr_bit_ == 8) {
			data_.emplace_back(curr_byte_);
			curr_bit_ = 0;
		}
	}

	void write_n_zero_bits(std::size_t n) {
		if (n + curr_bit_ >= 8) {
			auto remainingBits = 8 - curr_bit_;
			n -= remainingBits;
			curr_byte_ <<= remainingBits;
			data_.emplace_back(curr_byte_);
			curr_bit_ = 0;
		}

		while (n >= 8) {
			data_.emplace_back(0);
			n -= 8;
		}

		curr_byte_ <<= n;
		curr_bit_ += n;
	}

	void write_n_one_bits(std::size_t n) {
		constexpr std::uint8_t all_ones = std::numeric_limits<std::uint8_t>::max();
		if (n + curr_bit_ >= 8) {
			auto remainingBits = 8 - curr_bit_;
			n -= remainingBits;
			curr_byte_ <<= remainingBits;
			curr_byte_ |= all_ones >> (8 - remainingBits);
			data_.emplace_back(curr_byte_);
			curr_bit_ = 0;
		}

		while (n >= 8) {
			data_.emplace_back(all_ones);
			n -= 8;
		}

		curr_byte_ = (curr_byte_ << n) | (all_ones >> (8 - n));
		curr_bit_ += n;
	}

	void pad() {
		while (curr_bit_ > 0) {
			write_bit<0>();
		}
	}

	std::vector<std::uint8_t> get_data() const {
		return data_;
	}


private:
	std::vector<std::uint8_t> data_;
	std::uint8_t curr_byte_ = 0;
	std::size_t curr_bit_ = 0;
};

#endif