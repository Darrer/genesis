#ifndef __M68K_BUS_SCHEDULER_H__
#define __M68K_BUS_SCHEDULER_H__

#include <queue>
#include <variant>
#include <optional>
#include <functional>

#include "prefetch_queue.hpp"
#include "bus_manager.hpp"


namespace genesis::m68k
{

// TODO: move away
enum class size_type : std::uint8_t
{
	BYTE,
	WORD,
	LONG,
};


// TODO: maybe bus_scheduler?
class bus_scheduler
{
public:
	constexpr static const std::size_t queue_size = 20;
	using on_read_complete = std::function<void(std::uint32_t /*data*/, size_type)>;

private:
	enum class op_type : std::uint8_t
	{
		READ,
		READ_IMM,
		WRITE,
		PREFETCH_ONE,
		PREFETCH_TWO,
		PREFETCH_IRC,
		WAIT,
	};

	struct read_operation
	{
		std::uint32_t addr;
		size_type size;
		on_read_complete on_complete;
	};

	struct read_imm_operation
	{
		size_type size;
		on_read_complete on_complete;
	};

	struct write_operation
	{
		std::uint32_t addr;
		std::uint32_t data;
		size_type size;
	};

	struct wait_operation
	{
		std::uint16_t cycles;
	};

	struct operation
	{
		op_type type;
		std::variant<read_operation, read_imm_operation,
			write_operation, wait_operation> op;
	};

public:
	bus_scheduler(m68k::cpu_registers& regs, m68k::bus_manager& busm, m68k::prefetch_queue& pq);
	~bus_scheduler() { }

	void cycle();
	bool is_idle() const;
	void reset();


	template<class Callable>
	void read(std::uint32_t addr, size_type size, Callable on_complete)
	{
		static_assert(sizeof(Callable) <= 8);
		read_impl(addr, size, on_complete);
	}

	template<class Callable>
	void read_imm(size_type size, Callable on_complete)
	{
		static_assert(sizeof(Callable) <= 8);
		read_imm_impl(size, on_complete);
	}

	void write(std::uint32_t addr, std::uint32_t data, size_type size);

	void prefetch_one();
	void prefetch_two();
	void prefetch_irc();

	void wait(std::uint16_t cycles);

private:
	void read_impl(std::uint32_t addr, size_type size, on_read_complete on_complete = nullptr);
	void read_imm_impl(size_type size, on_read_complete on_complete = nullptr);

	void on_read_finish();
	bool current_op_is_over() const;
	void start_operation(operation);

private:
	m68k::cpu_registers& regs;
	m68k::bus_manager& busm;
	m68k::prefetch_queue& pq;

	// TODO: replace to stack-allocated queue
	std::queue<operation> queue;
	std::optional<operation> current_op;
	std::uint32_t data = 0;
	std::uint16_t curr_wait_cycles = 0;
};


};

#endif // __M68K_BUS_SCHEDULER_H__
