#ifndef __M68K_BUS_SCHEDULER_H__
#define __M68K_BUS_SCHEDULER_H__

#include <queue>
#include <variant>
#include <optional>
#include <functional>

#include "m68k/cpu_registers.hpp"
#include "prefetch_queue.hpp"
#include "bus_manager.hpp"


namespace genesis::m68k
{

enum class order : std::uint8_t
{
	lsw_first, // least significant word first
	msw_first, // most significant word first
};

// TODO: maybe back to scheduler?
class bus_scheduler
{
private:
	// all callbacks are restricted in size to the size of the pointer
	// this is required for std::function small-size optimizations
	// (though it's not guaranteed by standard, so we purely rely on implementation)
	constexpr const static std::size_t max_callable_size = sizeof(void*);

public:
	using on_read_complete = std::function<void(std::uint32_t /*data*/, size_type)>;

public:
	bus_scheduler(m68k::cpu_registers& regs, m68k::bus_manager& busm, m68k::prefetch_queue& pq);
	~bus_scheduler() = default;

	void cycle();
	void post_cycle();
	bool is_idle() const;
	void reset();

	template<class Callable>
	void read(std::uint32_t addr, size_type size, Callable on_complete)
	{
		static_assert(sizeof(Callable) <= max_callable_size);
		read_impl(addr, size, on_complete);
	}

	template<class Callable>
	void read_imm(size_type size, Callable on_complete)
	{
		static_assert(sizeof(Callable) <= max_callable_size);
		read_imm_impl(size, on_complete);
	}

	void write(std::uint32_t addr, std::uint32_t data, size_type size, order order = order::lsw_first);

	void prefetch_one();
	void prefetch_two();
	void prefetch_two_with_gap();
	void prefetch_irc();

	void wait(std::uint16_t cycles);

	template<class Callable>
	void call(Callable cb)
	{
		static_assert(sizeof(Callable) <= max_callable_size);
		call_impl(cb);
	}

private:
	enum class op_type : std::uint8_t
	{
		READ,
		READ_IMM,
		WRITE,
		PREFETCH_ONE,
		PREFETCH_TWO,
		PREFETCH_TWO_WITH_GAP,
		PREFETCH_IRC,
		WAIT,
		CALL,
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

	using callback = std::function<void()>;
	struct call_operation
	{
		callback cb;
	};

	struct operation
	{
		op_type type;
		std::variant<read_operation, read_imm_operation,
			write_operation, wait_operation, call_operation> op;
	};

private:
	void read_impl(std::uint32_t addr, size_type size, on_read_complete on_complete = nullptr);
	void read_imm_impl(size_type size, on_read_complete on_complete = nullptr);
	void call_impl(callback);

	void on_read_finished();
	bool current_op_is_over() const;
	void start_operation(operation);
	void run_call_operations();

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
