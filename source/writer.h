#ifndef WRITER_H
#define WRITER_H

#include <cstdio>
#include <memory>
#include <vector>

class Writer {
public:
	Writer() {}

	virtual ~Writer() {}

	virtual std::size_t write(const std::uint8_t* from, std::size_t n) = 0;
	virtual bool write_byte(std::uint8_t byte) = 0;

	virtual std::vector<std::uint8_t> get_data() = 0;

	virtual void rewind() = 0;
	virtual std::size_t num_bytes_written() = 0;
	virtual bool error() = 0;
};

class FileWriter : public Writer {
public:
	FileWriter(const std::string& file_path);
	~FileWriter();

	std::size_t write(const std::uint8_t* from, std::size_t n) override;
	bool write_byte(std::uint8_t byte) override;

	std::vector<std::uint8_t> get_data() override;

	void rewind() override;
	std::size_t num_bytes_written() override;
	bool error() override;

private:
	FILE* fptr_ = nullptr;
	std::vector<char> file_buffer_; // Used to replace the default file buffer for reads/writes to improve performance.
	const std::string file_path_;
};

class MemoryWriter : public Writer {
public:
	MemoryWriter();
	MemoryWriter(std::size_t initial_capacity);

	~MemoryWriter() {
	}

	std::size_t write(const std::uint8_t* from, std::size_t n) override;
	bool write_byte(std::uint8_t byte) override;

	std::vector<std::uint8_t> get_data() override;

	void rewind() override;
	std::size_t num_bytes_written() override;
	bool error() override;

private:
	std::vector<std::uint8_t> data_;
	std::size_t curr_byte_ = 0;

};

class StreamWriter : public Writer {
public:
	StreamWriter();
	~StreamWriter();

	std::size_t write(const std::uint8_t* from, std::size_t n) override;
	bool write_byte(std::uint8_t byte) override;

	std::vector<std::uint8_t> get_data() override;

	void rewind() override;
	std::size_t num_bytes_written() override;
	bool error() override;

private:
	std::unique_ptr<MemoryWriter> writer_;
};

#endif
