#ifndef UDA_SOURCE_CLIENT2_PERTHREADSINGLETON_H
#define UDA_SOURCE_CLIENT2_PERTHREADSINGLETON_H

#include <mutex>

#include "client.hpp"

namespace uda
{
namespace client
{

class ThreadClient
{
  public:
    static uda::client::Client& instance();

  private:
    ThreadClient() = default;
    ~ThreadClient() = default;

    static uda::client::Client* instance_;
    static std::once_flag init_flag_;

    static void init_client();
};

[[nodiscard]] uda::client_server::DataBlock* get_data_block(int handle);
[[nodiscard]] char* get_data(int handle);
[[nodiscard]] char* get_synthetic_data(int handle);
void set_synthetic_data(int handle, char* data);
[[nodiscard]] int get_data_status(int handle);
[[nodiscard]] int get_signal_status(int handle);
[[nodiscard]] int get_data_num(int handle);
[[nodiscard]] int get_data_type(int handle);
void get_error_model(int handle, int* model, int* param_n, float* params);
[[nodiscard]] int get_dim_num(int handle, int n_dim);
[[nodiscard]] char* get_dim_data(int handle, int n_dim);
[[nodiscard]] int get_dim_type(int handle, int n_dim);
[[nodiscard]] char* get_synthetic_dim_data(int handle, int n_dim);
void set_synthetic_dim_data(int handle, int n_dim, char* data);
[[nodiscard]] char* get_data_err_hi(int handle);
[[nodiscard]] char* get_data_err_lo(int handle);
[[nodiscard]] int get_data_err_asymmetry(int handle);
[[nodiscard]] char* get_dim_err_hi(int handle, int n_dim);
[[nodiscard]] char* get_dim_err_lo(int handle, int n_dim);
[[nodiscard]] int get_dim_err_asymmetry(int handle, int n_dim);
void set_dim_err_lo(int handle, int n_dim, char* err_lo);
void set_dim_err_type(int handle, int n_dim, int type);
void set_dim_err_asymmetry(int handle, int n_dim, int asymmetry);
[[nodiscard]] int get_rank(int handle);
void set_data_err_lo(int handle, char* err_lo);
void set_data_err_type(int handle, int type);
void set_data_err_asymmetry(int handle, int asymmetry);

} // namespace client
} // namespace uda

#endif // UDA_SOURCE_CLIENT2_PERTHREADSINGLETON_H
