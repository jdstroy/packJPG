/*
This file contains special classes for bitwise
reading and writing of arrays
*/

#include "bitops.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <vector>

#if defined(_WIN32) || defined(WIN32)
#include <fcntl.h>
#include <io.h>
#endif

BitReader::BitReader(const std::vector<std::uint8_t>& bits) :
	data(bits),
	cbyte(std::begin(data)),
	eof_(bits.empty()) {}

BitReader::~BitReader() {}

/* -----------------------------------------------
	reads n bits from abitreader
	----------------------------------------------- */

unsigned int BitReader::read(int nbits) {
	unsigned int retval = 0;

	// safety check for eof
	if (eof()) {
		overread_ = true;
		return 0;
	}

	while (nbits >= cbit) {
		nbits -= cbit;
		retval |= (bitops::RBITS(*cbyte, cbit) << nbits);
		cbit = 8;
		++cbyte;
		if (cbyte == std::end(data)) {
			eof_ = true;
			return retval;
		}
	}

	if (nbits > 0) {
		retval |= (MBITS(*cbyte, cbit, (cbit - nbits)));
		cbit -= nbits;
	}

	return retval;
}

std::uint8_t BitReader::read_bit() {
	// safety check for eof
	if (eof()) {
		overread_ = true;
		return 0;
	}

	// read one bit
	std::uint8_t bit = bitops::BITN(*cbyte, --cbit);
	if (cbit == 0) {
		++cbyte;
		if (cbyte == std::end(data)) {
			eof_ = true;
		}
		cbit = 8;
	}

	return bit;
}

/* -----------------------------------------------
	to skip padding from current byte
	----------------------------------------------- */

std::uint8_t BitReader::unpad(std::uint8_t fillbit) {
	if ((cbit == 8) || eof()) {
		return fillbit;
	} else {
		fillbit = read(1);
		if (cbit < 8) {
			++cbyte;
			cbit = 8;
			eof_ = cbyte == std::end(data);
		}
	}

	return fillbit;
}

bool BitReader::eof() const {
	return eof_;
}

bool BitReader::overread() const {
	return overread_;
}

BitWriter::BitWriter(int size) : data(std::max(size, 65536)) {}

BitWriter::~BitWriter() {}

/* -----------------------------------------------
	writes n bits to abitwriter
	----------------------------------------------- */

void BitWriter::write(unsigned int val, int nbits) {
	// test if pointer beyond flush treshold
	if (cbyte > (data.size() - 5)) {
		data.resize(data.size() * 2);
	}

	// write data
	while (nbits >= cbit) {
		data[cbyte] |= (MBITS32(val, nbits, (nbits - cbit)));
		nbits -= cbit;
		cbyte++;
		cbit = 8;
	}

	if (nbits > 0) {
		data[cbyte] |= ((RBITS32(val, nbits)) << (cbit - nbits));
		cbit -= nbits;
	}
}

/* -----------------------------------------------
	writes one bit to abitwriter
	----------------------------------------------- */

void BitWriter::write_bit(std::uint8_t bit) {

	// write data
	if (bit) {
		data[cbyte] |= 0x1 << (--cbit);
	} else {
		--cbit;
	}
	if (cbit == 0) {
		// test if pointer beyond flush treshold
		cbyte++;
		if (cbyte > (data.size() - 5)) {
			data.resize(data.size() * 2);
		}
		cbit = 8;
	}
}

/* -----------------------------------------------
	Sets the fillbit for padding data.
   ----------------------------------------------- */
void BitWriter::set_fillbit(std::uint8_t fillbit) {
	fillbit_ = fillbit;
}


/* -----------------------------------------------
	pads data using fillbit
	----------------------------------------------- */

void BitWriter::pad() {
	while (cbit < 8) {
		write(fillbit_, 1);
	}
}

std::vector<std::uint8_t> BitWriter::get_data() {
	pad(); // Pad the last bits of the data before returning it.
	data.resize(cbyte);
	return data;
}

/* -----------------------------------------------
	gets size of data array from abitwriter
	----------------------------------------------- */

int BitWriter::getpos() const {
	return cbyte;
}

FileReader::FileReader(const std::string& file_path) : file_path_(file_path) {
	fptr_ = fopen(file_path.c_str(), "rb");
	if (fptr_ != nullptr) {
		file_buffer_.reserve(32768);
		std::setvbuf(fptr_, file_buffer_.data(), _IOFBF, file_buffer_.capacity());
	} else {
		throw std::runtime_error("Unable to open " + file_path);
	}
}

FileReader::~FileReader() {
	if (fptr_ != nullptr) {
		fclose(fptr_);
	}
}

std::size_t FileReader::read(std::uint8_t* to, std::size_t num_to_read) {
	return fread(to, sizeof to[0], num_to_read, fptr_);
}

std::size_t FileReader::read(std::vector<std::uint8_t>& into, std::size_t num_to_read, std::size_t offset) {
	return read(into.data() + offset, num_to_read);
}

std::uint8_t FileReader::read_byte() {
	const int val = fgetc(fptr_);
	if (val != EOF) {
		return static_cast<std::uint8_t>(val);
	} else {
		throw std::runtime_error("No bytes left in " + file_path_ + " to read!");
	}
}

bool FileReader::read_byte(std::uint8_t* to) {
	const int val = fgetc(fptr_);
	*to = val;
	return val != EOF;
}

void FileReader::rewind() {
	fseek(fptr_, 0, SEEK_SET);
}

std::size_t FileReader::num_bytes_read() {
	return ftell(fptr_);
}

std::size_t FileReader::get_size() {
	const auto position = num_bytes_read();
	fseek(fptr_, 0, SEEK_END);
	const auto size = ftell(fptr_);
	fseek(fptr_, position, SEEK_SET);
	return size;
}

std::vector<std::uint8_t> FileReader::get_data() {
	auto position = num_bytes_read();
	std::vector<std::uint8_t> data_copy(get_size());
	fseek(fptr_, 0, SEEK_SET);
	fread(data_copy.data(), sizeof data_copy[0], data_copy.capacity(), fptr_);
	fseek(fptr_, position, SEEK_SET);
	return data_copy;
}

bool FileReader::error() {
	return fptr_ == nullptr || ferror(fptr_) != 0;
}

bool FileReader::end_of_reader() {
	return feof(fptr_) != 0;
}

MemoryReader::MemoryReader(const std::vector<std::uint8_t>& bytes) :
	data_(bytes),
	cbyte_(std::begin(data_)),
	eof_(bytes.empty()) {}

std::size_t MemoryReader::read(std::uint8_t* to, std::size_t num_to_read) {
	if (num_to_read == 0 || to == nullptr) {
		return 0;
	}
	auto numAvailable = std::distance(cbyte_, std::end(data_));
	int numRead = std::min(static_cast<std::size_t>(numAvailable), num_to_read);
	auto end = std::next(cbyte_, numRead);
	std::copy(cbyte_, end, to);
	cbyte_ = end;
	eof_ = cbyte_ == std::end(data_);
	return numRead;
}

std::size_t MemoryReader::read(std::vector<std::uint8_t>& into, std::size_t n, std::size_t offset) {
	const std::size_t num_available = get_size() - num_bytes_read(); // The number of bytes in the reader not yet read.
	const std::size_t num_to_read = std::min(n, num_available); // How many bytes will be read.
	if (into.size() < num_to_read + offset) {
		into.resize(num_to_read + offset);
	}

	const auto end = std::next(cbyte_, num_to_read);
	const auto write_start = std::next(std::begin(into), offset);
	std::copy(cbyte_, end, write_start);
	cbyte_ = end;
	return num_to_read;
}

std::uint8_t MemoryReader::read_byte() {
	if (cbyte_ == std::end(data_)) {
		throw std::runtime_error("No bytes left to read");
	} else {
		std::uint8_t the_byte = *cbyte_;
		++cbyte_;
		eof_ = cbyte_ == std::end(data_);
		return the_byte;
	}
}

bool MemoryReader::read_byte(std::uint8_t* byte) {
	if (cbyte_ == std::end(data_)) {
		eof_ = true;
		return false;
	} else {
		*byte = *cbyte_;
		++cbyte_;
		eof_ = cbyte_ == std::end(data_);
		return true;
	}
}

void MemoryReader::rewind() {
	cbyte_ = std::begin(data_);
	eof_ = cbyte_ == std::end(data_);
}

std::size_t MemoryReader::num_bytes_read() {
	return std::distance(std::begin(data_), cbyte_);
}

std::size_t MemoryReader::get_size() {
	return data_.size();
}

std::vector<std::uint8_t> MemoryReader::get_data() {
	return data_;
}

bool MemoryReader::error() {
	return false;
}

bool MemoryReader::end_of_reader() {
	return eof_;
}

StreamReader::StreamReader() {
#if defined(_WIN32) || defined(WIN32)
	_setmode(_fileno(stdin), _O_BINARY);
#endif
	// read whole stream into memory buffer
	auto writer = std::make_unique<MemoryWriter>();
	constexpr auto buffer_capacity = 1024 * 1024;
	std::vector<std::uint8_t> buffer(buffer_capacity);

	auto bytes_read = fread(buffer.data(), sizeof buffer[0], buffer_capacity, stdin);
	while (bytes_read > 0) {
		writer->write(buffer.data(), bytes_read);
		bytes_read = fread(buffer.data(), sizeof buffer[0], buffer_capacity, stdin);
	}
	const auto bytes = writer->get_data();

	reader_ = std::make_unique<MemoryReader>(bytes);
}

std::size_t StreamReader::read(std::uint8_t* to, std::size_t num_to_read) {
	return reader_->read(to, num_to_read);
}

std::size_t StreamReader::read(std::vector<std::uint8_t>& into, std::size_t num_to_read, std::size_t offset) {
	return reader_->read(into, num_to_read, offset);
}

std::uint8_t StreamReader::read_byte() {
	try {
		return reader_->read_byte();
	} catch (const std::exception&) {
		throw;
	}
}

bool StreamReader::read_byte(std::uint8_t* to) {
	return reader_->read_byte(to);
}

void StreamReader::rewind() {
	reader_->rewind();
}

std::size_t StreamReader::num_bytes_read() {
	return reader_->num_bytes_read();
}

std::size_t StreamReader::get_size() {
	return reader_->get_size();
}

std::vector<std::uint8_t> StreamReader::get_data() {
	return reader_->get_data();
}

bool StreamReader::error() {
	return reader_->error();
}

bool StreamReader::end_of_reader() {
	return reader_->end_of_reader();
}

FileWriter::FileWriter(const std::string& file_path) : file_path_(file_path) {
	fptr_ = fopen(file_path.c_str(), "wb");
	if (fptr_ != nullptr) {
		file_buffer_.reserve(32768);
		std::setvbuf(fptr_, file_buffer_.data(), _IOFBF, file_buffer_.capacity());
	} else {
		throw std::runtime_error("Unable to open " + file_path_ + " for writing.");
	}

}

FileWriter::~FileWriter() {
	if (fptr_ != nullptr) {
		fflush(fptr_);
		fclose(fptr_);
	}
}

std::size_t FileWriter::write(const std::uint8_t* from, std::size_t n) {
	return fwrite(from, sizeof from[0], n, fptr_);
}

bool FileWriter::write_byte(std::uint8_t byte) {
	return fputc(byte, fptr_) == byte;
}

std::vector<std::uint8_t> FileWriter::get_data() {
	fflush(fptr_);
	auto file_reader = fopen(file_path_.c_str(), "rb");
	if (file_reader == nullptr) {
		return std::vector<std::uint8_t>();
	}
	std::vector<std::uint8_t> data_copy(num_bytes_written());
	fseek(file_reader, 0, SEEK_SET);
	std::fread(data_copy.data(), sizeof(uint8_t), data_copy.size(), file_reader);
	fclose(file_reader);
	return data_copy;
}

void FileWriter::rewind() {
	fseek(fptr_, 0, SEEK_SET);
}

std::size_t FileWriter::num_bytes_written() {
	return ftell(fptr_);
}

bool FileWriter::error() {
	return fptr_ == nullptr || ferror(fptr_);
}

MemoryWriter::MemoryWriter() : data_(65536) {}

MemoryWriter::MemoryWriter(std::size_t initial_capacity)
	: data_(std::max(initial_capacity, static_cast<std::size_t>(65536))) {}

std::size_t MemoryWriter::write(const std::uint8_t* from, std::size_t n) {
	// make sure that pointer doesn't get beyond flush threshold
	while (curr_byte_ + n >= data_.size()) {
		data_.resize(data_.size() * 2);
	}

	std::copy(from, from + n, std::next(std::begin(data_), curr_byte_));
	curr_byte_ += n;
	return n;
}

bool MemoryWriter::write_byte(std::uint8_t byte) {
	if (curr_byte_ == data_.size()) {
		data_.resize(data_.size() * 2);
	}

	data_[curr_byte_] = byte;
	curr_byte_++;
	return true;
}

std::vector<std::uint8_t> MemoryWriter::get_data() {
	std::vector<std::uint8_t> copy(data_.begin(), data_.begin() + curr_byte_);
	return copy;
}

void MemoryWriter::rewind() {
	curr_byte_ = 0;
}

std::size_t MemoryWriter::num_bytes_written() {
	return curr_byte_;
}

bool MemoryWriter::error() {
	return false;
}

StreamWriter::StreamWriter() {
	writer_ = std::make_unique<MemoryWriter>();
}

StreamWriter::~StreamWriter() {
#if defined(_WIN32) || defined(WIN32)
	_setmode(_fileno(stdout), _O_BINARY);
#endif
	const auto& data = writer_->get_data();
	fwrite(data.data(), sizeof data[0], data.size(), stdout);
}

std::size_t StreamWriter::write(const std::uint8_t* from, std::size_t n) {
	return writer_->write(from, n);
}

bool StreamWriter::write_byte(std::uint8_t byte) {
	return writer_->write_byte(byte);
}

std::vector<std::uint8_t> StreamWriter::get_data() {
	return writer_->get_data();
}

void StreamWriter::rewind() {
	writer_->rewind();
}

std::size_t StreamWriter::num_bytes_written() {
	return writer_->num_bytes_written();
}

bool StreamWriter::error() {
	return writer_->error();
}