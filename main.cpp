#include <array>
#include <iostream>
#include <shared_mutex>
#include <thread>

constexpr size_t philosophers_number = 5;
struct fork final {
public:
    enum class state : int8_t
    {
        free = 1 << 0,
        owned = 1 << 1,
    };

private:
    mutable std::shared_mutex mutex_;
    state state_;

public:
    fork() : state_(state::free) {}
    [[nodiscard]] state getState() const {
        std::shared_lock lock(mutex_);
        return state_;
    }

    void setState(state state) {
        std::unique_lock lock(mutex_);
        state_ = state;
    }
};

struct context final
{
    std::mutex mutex;
    std::array<struct fork, philosophers_number> forks{};
};

std::mutex logMutex;
void log(std::string_view message)
{
    std::lock_guard<std::mutex> lock(logMutex);
    std::cout << message << std::endl;
}

class philosopher final {
private:
    struct fork_indices final
    {
        size_t left;
        size_t right;
    };

    [[nodiscard]]
    fork_indices get_forks_indices() const
    {
        return {id_ - 1, id_ % philosophers_number};
    }

    void eat(const fork_indices& indices)
    {
        auto [left, right] = indices;
        auto& [forks_mutex, forks] = context_;
        std::unique_lock lock(forks_mutex);
        forks[left].setState(fork::state::owned);
        forks[right].setState(fork::state::owned);
        log("Philosopher#" + std::to_string(id_) + " starts dinner!");
        std::this_thread::sleep_for(std::chrono::seconds(10));
        forks[left].setState(fork::state::free);
        forks[right].setState(fork::state::free);
        log("Philosopher#" + std::to_string(id_) + " finished dinner!");
    }

    void wait(std::string_view message)
    {
        log(message);
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

public:
    philosopher(size_t id, context &context) : id_(id), context_(context)
    {
    }
    void try_to_eat() {
        auto indices = get_forks_indices();
        auto[left, right] = indices;
        auto& forks = context_.forks;

        while (true)
        {
            if (forks[left].getState() == fork::state::free && forks[right].getState() == fork::state::free)
            {
               eat(indices);
            }

            if (forks[left].getState() == fork::state::free && forks[right].getState() == fork::state::owned)
            {
                wait("Philosopher #" + std::to_string(id_) + " can take left fork but right fork is not free.");
            }

            if (forks[right].getState() == fork::state::free && forks[left].getState() == fork::state::owned)
            {
                wait("Philosopher #" + std::to_string(id_) + " can take right fork but left fork is not free.");
            }

            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
    }

private:
    size_t id_;
    context &context_;
};

int main() {
    context context;
    philosopher p1(1, context);
    philosopher p2(2, context);
    philosopher p3(3, context);
    philosopher p4(4, context);
    philosopher p5(5, context);

    std::thread t(&philosopher::try_to_eat, &p1);
    std::thread t1(&philosopher::try_to_eat, &p2);
    std::thread t2(&philosopher::try_to_eat, &p3);
    std::thread t3(&philosopher::try_to_eat, &p4);
    std::thread t4(&philosopher::try_to_eat, &p5);

    t.join();
    t1.join();
    t2.join();
    t3.join();
    t4.join();
}
