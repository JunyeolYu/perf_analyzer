// Copyright 2020-2025, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#pragma once

#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>

#include <chrono>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "../constants.h"
#include "../metrics.h"
#include "../perf_analyzer_exception.h"
#include "ipc.h"

#ifdef TRITON_ENABLE_GPU
#include "../cuda_runtime_library_manager.h"
#endif  // TRITON_ENABLE_GPU

namespace pa = triton::perfanalyzer;

namespace triton { namespace perfanalyzer { namespace clientbackend {

#define RETURN_IF_CB_ERROR(S)                                         \
  do {                                                                \
    const triton::perfanalyzer::clientbackend::Error& status__ = (S); \
    if (!status__.IsOk()) {                                           \
      return status__;                                                \
    }                                                                 \
  } while (false)

#define RETURN_IF_ERROR(S)                                     \
  do {                                                         \
    triton::perfanalyzer::clientbackend::Error status__ = (S); \
    if (!status__.IsOk()) {                                    \
      return status__;                                         \
    }                                                          \
  } while (false)

#define FAIL_IF_ERR(X, MSG)                                        \
  {                                                                \
    triton::perfanalyzer::clientbackend::Error err = (X);          \
    if (!err.IsOk()) {                                             \
      std::cerr << "error: " << (MSG) << ": " << err << std::endl; \
      exit(err.Err());                                             \
    }                                                              \
  }                                                                \
  while (false)

#define THROW_IF_ERROR(S, MSG)                                          \
  do {                                                                  \
    triton::perfanalyzer::clientbackend::Error status__ = (S);          \
    if (!status__.IsOk()) {                                             \
      std::cerr << "error: " << (MSG) << ": " << status__ << std::endl; \
      throw PerfAnalyzerException(GENERIC_ERROR);                       \
    }                                                                   \
  } while (false)

//==============================================================================
/// Error status reported by backends
///
class Error {
 public:
  /// Create an error
  explicit Error();

  /// Create an error with the specified message and error code.
  /// \param msg The message for the error
  /// \param err The error code for the error
  explicit Error(const std::string& msg, const uint32_t err);

  /// Create an error with the specified message.
  /// \param msg The message for the error
  explicit Error(const std::string& msg);

  /// Accessor for the message of this error.
  /// \return The message for the error. Empty if no error.
  const std::string& Message() const { return msg_; }

  /// Accessor for the error code.
  /// \return The error code for the error. 0 if no error.
  const uint32_t Err() const { return error_; }

  /// Does this error indicate OK status?
  /// \return True if this error indicates "ok"/"success", false if
  /// error indicates a failure.
  bool IsOk() const { return error_ == 0; }

  /// Convenience "success" value. Can be used as Error::Success to
  /// indicate no error.
  static const Error Success;

  /// Convenience "failure" value. Can be used as Error::Failure to
  /// indicate a generic error.
  static const Error Failure;

 private:
  friend std::ostream& operator<<(std::ostream&, const Error&);
  std::string msg_{""};
  uint32_t error_{pa::SUCCESS};
};

//===================================================================================

class ClientBackend;
class InferInput;
class InferRequestedOutput;
class InferResult;

enum BackendKind {
  TRITON = 0,
  TENSORFLOW_SERVING = 1,
  TORCHSERVE = 2,
  TRITON_C_API = 3,
  OPENAI = 4,
  DYNAMIC_GRPC = 5
};
std::string BackendKindToString(const BackendKind kind);

enum ProtocolType { HTTP = 0, GRPC = 1, UNKNOWN = 2 };
enum GrpcCompressionAlgorithm {
  COMPRESS_NONE = 0,
  COMPRESS_DEFLATE = 1,
  COMPRESS_GZIP = 2
};
enum class TensorFormat { BINARY, JSON, UNKNOWN };
typedef std::map<std::string, std::string> Headers;

using OnCompleteFn = std::function<void(InferResult*)>;
using ModelIdentifier = std::pair<std::string, std::string>;

struct InferStat {
  /// Total number of requests completed.
  size_t completed_request_count;

  /// Time from the request start until the response is completely
  /// received.
  uint64_t cumulative_total_request_time_ns;

  /// Time from the request start until the last byte is sent.
  uint64_t cumulative_send_time_ns;

  /// Time from receiving first byte of the response until the
  /// response is completely received.
  uint64_t cumulative_receive_time_ns;

  /// Create a new InferStat object with zero-ed statistics.
  InferStat()
      : completed_request_count(0), cumulative_total_request_time_ns(0),
        cumulative_send_time_ns(0), cumulative_receive_time_ns(0)
  {
  }
};

// Per model statistics
struct ModelStatistics {
  uint64_t success_count_;
  uint64_t inference_count_;
  uint64_t execution_count_;
  uint64_t queue_count_;
  uint64_t compute_input_count_;
  uint64_t compute_infer_count_;
  uint64_t compute_output_count_;
  uint64_t cache_hit_count_;
  uint64_t cache_miss_count_;
  uint64_t cumm_time_ns_;
  uint64_t queue_time_ns_;
  uint64_t compute_input_time_ns_;
  uint64_t compute_infer_time_ns_;
  uint64_t compute_output_time_ns_;
  uint64_t cache_hit_time_ns_;
  uint64_t cache_miss_time_ns_;
};

///
/// Structure to hold Request parameter data for Inference Request.
///
struct RequestParameter {
  std::string name;
  std::string value;
  std::string type;
};

//==============================================================================
/// Structure to hold options for Inference Request.
///
struct InferOptions {
  explicit InferOptions(const std::string& model_name)
      : model_name_(model_name), model_version_(""), request_id_(""),
        sequence_id_(0), sequence_id_str_(""), sequence_start_(false),
        sequence_end_(false), triton_enable_empty_final_response_(true)
  {
  }
  /// The name of the model to run inference.
  std::string model_name_;
  /// The version of the model.
  std::string model_version_;
  /// The model signature name for TF models.
  std::string model_signature_name_;
  /// An identifier for the request.
  std::string request_id_;
  /// The unique identifier for the sequence being represented by the
  /// object. Default value is 0 which means that the request does not
  /// belong to a sequence. If this value is set, then sequence_id_str_
  /// MUST be set to "".
  uint64_t sequence_id_;
  /// The unique identifier for the sequence being represented by the
  /// object. Default value is "" which means that the request does not
  /// belong to a sequence. If this value is set, then sequence_id_ MUST
  /// be set to 0.
  std::string sequence_id_str_;
  /// Indicates whether the request being added marks the start of the
  /// sequence. Default value is False. This argument is ignored if
  /// 'sequence_id' is 0.
  bool sequence_start_;
  /// Indicates whether the request being added marks the end of the
  /// sequence. Default value is False. This argument is ignored if
  /// 'sequence_id' is 0.
  bool sequence_end_;
  /// Whether to tell Triton to enable an empty final response.
  bool triton_enable_empty_final_response_;

  /// Additional parameters to pass to the model
  std::unordered_map<std::string, RequestParameter> request_parameters_;
};

struct SslOptionsBase {
  bool ssl_grpc_use_ssl = false;
  std::string ssl_grpc_root_certifications_file = "";
  std::string ssl_grpc_private_key_file = "";
  std::string ssl_grpc_certificate_chain_file = "";
  long ssl_https_verify_peer = 1L;
  long ssl_https_verify_host = 2L;
  std::string ssl_https_ca_certificates_file = "";
  std::string ssl_https_client_certificate_file = "";
  std::string ssl_https_client_certificate_type = "";
  std::string ssl_https_private_key_file = "";
  std::string ssl_https_private_key_type = "";
};

//
// The object factory to create client backends to communicate with the
// inference service
//
class ClientBackendFactory {
 public:
  /// Create a factory that can be used to construct Client Backends.
  /// \param kind The kind of client backend to create.
  /// \param url The inference server url and port.
  /// \param endpoint The endpoint on the inference server to send requests to
  /// \param protocol The protocol type used.
  /// \param ssl_options The SSL options used with client backend.
  /// \param compression_algorithm The compression algorithm to be used
  /// on the grpc requests.
  /// \param http_headers Map of HTTP headers. The map key/value
  /// indicates the header name/value. The headers will be included
  /// with all the requests made to server using this client.
  /// \param triton_server_path Only for C api backend. Lbrary path to
  /// path to the top-level Triton directory (which is typically
  /// /opt/tritonserver) Must contain libtritonserver.so.
  /// \param model_repository_path Only for C api backend. Path to model
  /// repository which contains the desired model.
  /// \param verbose Enables the verbose mode.
  /// \param metrics_url The inference server metrics url and port.
  /// \param input_tensor_format The Triton inference request input tensor
  /// format.
  /// \param output_tensor_format The Triton inference response output tensor
  /// format.
  /// \param factory Returns a new ClientBackend object.
  /// \return Error object indicating success or failure.
  static Error Create(
      const BackendKind kind, const std::string& url,
      const std::string& endpoint, const ProtocolType protocol,
      const SslOptionsBase& ssl_options,
      const std::map<std::string, std::vector<std::string>> trace_options,
      const GrpcCompressionAlgorithm compression_algorithm,
      std::shared_ptr<Headers> http_headers,
      const std::string& triton_server_path,
      const std::string& model_repository_path, const bool verbose,
      const std::string& metrics_url, const TensorFormat input_tensor_format,
      const TensorFormat output_tensor_format, const std::string& grpc_method,
      std::shared_ptr<ClientBackendFactory>* factory);

  const BackendKind& Kind();

  /// Create a ClientBackend.
  /// \param backend Returns a new Client backend object.
  virtual Error CreateClientBackend(std::unique_ptr<ClientBackend>* backend);

 private:
  ClientBackendFactory(
      const BackendKind kind, const std::string& url,
      const std::string& endpoint, const ProtocolType protocol,
      const SslOptionsBase& ssl_options,
      const std::map<std::string, std::vector<std::string>> trace_options,
      const GrpcCompressionAlgorithm compression_algorithm,
      const std::shared_ptr<Headers> http_headers,
      const std::string& triton_server_path,
      const std::string& model_repository_path, const bool verbose,
      const std::string& metrics_url, const TensorFormat input_tensor_format,
      const TensorFormat output_tensor_format, const std::string& grpc_method)
      : kind_(kind), url_(url), endpoint_(endpoint), protocol_(protocol),
        ssl_options_(ssl_options), trace_options_(trace_options),
        compression_algorithm_(compression_algorithm),
        http_headers_(http_headers), triton_server_path(triton_server_path),
        model_repository_path_(model_repository_path), verbose_(verbose),
        metrics_url_(metrics_url), input_tensor_format_(input_tensor_format),
        output_tensor_format_(output_tensor_format), grpc_method_(grpc_method)
  {
  }

  const BackendKind kind_;
  const std::string url_;
  const std::string endpoint_;
  const ProtocolType protocol_;
  const SslOptionsBase& ssl_options_;
  const std::map<std::string, std::vector<std::string>> trace_options_;
  const GrpcCompressionAlgorithm compression_algorithm_;
  std::shared_ptr<Headers> http_headers_;
  std::string triton_server_path;
  std::string model_repository_path_;
  const bool verbose_;
  const std::string metrics_url_{""};
  const TensorFormat input_tensor_format_{TensorFormat::UNKNOWN};
  const TensorFormat output_tensor_format_{TensorFormat::UNKNOWN};
  const std::string grpc_method_;


#ifndef DOCTEST_CONFIG_DISABLE
 protected:
  ClientBackendFactory()
      : kind_(BackendKind()), url_(""), protocol_(ProtocolType()),
        ssl_options_(SslOptionsBase()),
        trace_options_(std::map<std::string, std::vector<std::string>>()),
        compression_algorithm_(GrpcCompressionAlgorithm()), verbose_(false)
  {
  }
#endif
};

//
// Interface for interacting with an inference service
//
class ClientBackend {
 public:
  static Error Create(
      const BackendKind kind, const std::string& url,
      const std::string& endpoint, const ProtocolType protocol,
      const SslOptionsBase& ssl_options,
      const std::map<std::string, std::vector<std::string>> trace_options,
      const GrpcCompressionAlgorithm compression_algorithm,
      std::shared_ptr<Headers> http_headers, const bool verbose,
      const std::string& library_directory, const std::string& model_repository,
      const std::string& metrics_url, const TensorFormat input_tensor_format,
      const TensorFormat output_tensor_format, const std::string& grpc_method,
      std::unique_ptr<ClientBackend>* client_backend);

  /// Destructor for the client backend object
  virtual ~ClientBackend() = default;

  /// Get the backend kind
  BackendKind Kind() const { return kind_; }

  /// Get the server metadata from the server
  virtual Error ServerExtensions(std::set<std::string>* server_extensions);

  /// Get the model metadata from the server for specified name and
  /// version as rapidjson DOM object.
  virtual Error ModelMetadata(
      rapidjson::Document* model_metadata, const std::string& model_name,
      const std::string& model_version);

  /// Get the model config from the server for specified name and
  /// version as rapidjson DOM object.
  virtual Error ModelConfig(
      rapidjson::Document* model_config, const std::string& model_name,
      const std::string& model_version);

  /// Issues a synchronous inference request to the server.
  virtual Error Infer(
      InferResult** result, const InferOptions& options,
      const std::vector<InferInput*>& inputs,
      const std::vector<const InferRequestedOutput*>& outputs);

  /// Issues an asynchronous inference request to the server.
  virtual Error AsyncInfer(
      OnCompleteFn callback, const InferOptions& options,
      const std::vector<InferInput*>& inputs,
      const std::vector<const InferRequestedOutput*>& outputs);

  /// Established a stream to the server.
  virtual Error StartStream(OnCompleteFn callback, bool enable_stats);

  /// Issues an asynchronous inference request to the underlying stream.
  virtual Error AsyncStreamInfer(
      const InferOptions& options, const std::vector<InferInput*>& inputs,
      const std::vector<const InferRequestedOutput*>& outputs);

  /// Gets the client side inference statistics from the client library.
  virtual Error ClientInferStat(InferStat* infer_stat);

  /// Gets the server-side model inference statistics from the server.
  virtual Error ModelInferenceStatistics(
      std::map<ModelIdentifier, ModelStatistics>* model_stats,
      const std::string& model_name = "",
      const std::string& model_version = "");

  /// Gets the server-side metrics from the server.
  /// \param metrics Output metrics object.
  /// \return Error object indicating success or failure.
  virtual Error Metrics(Metrics& metrics);

  /// Unregisters all the shared memory from the server
  virtual Error UnregisterAllSharedMemory();

  /// Registers a system shared memory from the server
  virtual Error RegisterSystemSharedMemory(
      const std::string& name, const std::string& key, const size_t byte_size);

#ifdef TRITON_ENABLE_GPU
  /// Registers cuda shared memory to the server.
  virtual Error RegisterCudaSharedMemory(
      const std::string& name,
      const CUDARuntimeLibraryManager::cudaIpcMemHandle_t& handle,
      const size_t byte_size);
#endif  // TRITON_ENABLE_GPU

  /// Registers cuda memory to the server.
  virtual Error RegisterCudaMemory(
      const std::string& name, void* handle, const size_t byte_size);

  /// Registers a system memory location on the server.
  virtual Error RegisterSystemMemory(
      const std::string& name, void* memory_ptr, const size_t byte_size);

  //
  // Shared Memory Utilities
  //
  // FIXME: These should probably move to a common area with shm_utils not
  // tied specifically to inferenceserver. Create a shared memory region of
  // the size 'byte_size' and return the unique identifier.
  virtual Error CreateSharedMemoryRegion(
      std::string shm_key, size_t byte_size, int* shm_fd);

  // Mmap the shared memory region with the given 'offset' and 'byte_size' and
  // return the base address of the region.
  // \param shm_fd The int descriptor of the created shared memory region
  // \param offset The offset of the shared memory block from the start of the
  // shared memory region
  // \param byte_size The size in bytes of the shared memory region
  // \param shm_addr Returns the base address of the shared memory region
  // \return error Returns an error if unable to mmap shared memory region.
  virtual Error MapSharedMemory(
      int shm_fd, size_t offset, size_t byte_size, void** shm_addr);

  // Close the shared memory descriptor.
  // \param shm_fd The int descriptor of the created shared memory region
  // \return error Returns an error if unable to close shared memory descriptor.
  virtual Error CloseSharedMemory(int shm_fd);

  // Destroy the shared memory region with the given name.
  // \return error Returns an error if unable to unlink shared memory region.
  virtual Error UnlinkSharedMemoryRegion(std::string shm_key);

  // Munmap the shared memory region from the base address with the given
  // byte_size.
  // \return error Returns an error if unable to unmap shared memory region.
  virtual Error UnmapSharedMemory(void* shm_addr, size_t byte_size);

 protected:
  /// Constructor for client backend
  ClientBackend(const BackendKind kind);
  // The kind of the backend.
  const BackendKind kind_{TRITON};

#ifndef DOCTEST_CONFIG_DISABLE
 public:
  ClientBackend() = default;
#endif
};


//
// Interface for preparing the inputs for inference to the backend
//
class InferInput {
 public:
  /// Create a InferInput instance that describes a model input.
  /// \param infer_input Returns a new InferInput object.
  /// \param kind The kind of the associated client backend.
  /// \param name The name of input whose data will be described by this object.
  /// \param dims The shape of the input.
  /// \param datatype The datatype of the input.
  /// \return Error object indicating success or failure.
  static Error Create(
      InferInput** infer_input, const BackendKind kind, const std::string& name,
      const std::vector<int64_t>& dims, const std::string& datatype);

  virtual ~InferInput() = default;

  /// Gets name of the associated input tensor.
  /// \return The name of the tensor.
  const std::string& Name() const { return name_; }

  /// Gets datatype of the associated input tensor.
  /// \return The datatype of the tensor.
  const std::string& Datatype() const { return datatype_; }

  /// Gets the shape of the input tensor.
  /// \return The shape of the tensor.
  virtual const std::vector<int64_t>& Shape() const = 0;

  /// Set the shape of input associated with this object.
  /// \param dims the vector of dims representing the new shape
  /// of input.
  /// \return Error object indicating success or failure of the
  /// request.
  virtual Error SetShape(const std::vector<int64_t>& dims);

  /// Prepare this input to receive new tensor values. Forget any
  /// existing values that were set by previous calls to SetSharedMemory()
  /// or AppendRaw().
  /// \return Error object indicating success or failure.
  virtual Error Reset();

  /// Append tensor values for this input from a byte array.
  /// \param input The pointer to the array holding the tensor value.
  /// \param input_byte_size The size of the array in bytes.
  /// \return Error object indicating success or failure.
  virtual Error AppendRaw(const uint8_t* input, size_t input_byte_size);

  /// Set tensor values for this input by reference into a shared memory
  /// region.
  /// \param name The user-given name for the registered shared memory region
  /// where the tensor values for this input is stored.
  /// \param byte_size The size, in bytes of the input tensor data. Must
  /// match the size expected for the input shape.
  /// \param offset The offset into the shared memory region upto the start
  /// of the input tensor values. The default value is 0.
  /// \return Error object indicating success or failure
  virtual Error SetSharedMemory(
      const std::string& name, size_t byte_size, size_t offset = 0);

  /// Get access to the buffer holding raw input. Note the buffer is owned by
  /// InferInput instance. Users can copy out the data if required to extend
  /// the lifetime.
  /// \param buf Returns the pointer to the start of the buffer.
  /// \param byte_size Returns the size of buffer in bytes.
  /// \return Error object indicating success or failure of the
  /// request.
  virtual Error RawData(const uint8_t** buf, size_t* byte_size);

 protected:
  InferInput(
      const BackendKind kind, const std::string& name,
      const std::string& datatype_);

  const BackendKind kind_;
  const std::string name_;
  const std::string datatype_;
};


//
// Interface for preparing the inputs for inference to the backend
//
class InferRequestedOutput {
 public:
  virtual ~InferRequestedOutput() = default;

  /// Create a InferRequestedOutput instance that describes a model output being
  /// requested.
  /// \param infer_output Returns a new InferOutputGrpc object.
  /// \param kind The kind of the associated client backend.
  /// \param name The name of output being requested.
  /// \param datatype The datatype of the output
  /// \param class_count The number of classifications to be requested. The
  /// default value is 0 which means the classification results are not
  /// requested.
  /// \return Error object indicating success or failure.
  static Error Create(
      InferRequestedOutput** infer_output, const BackendKind kind,
      const std::string& name, const std::string& datatype,
      const size_t class_count = 0);

  /// Gets name of the associated output tensor.
  /// \return The name of the tensor.
  const std::string& Name() const { return name_; }

  /// Gets datatype of the associated output tensor.
  /// \return The datatype of the tensor
  const std::string& Datatype() const { return datatype_; }

  /// Set the output tensor data to be written to specified shared
  /// memory region.
  /// \param region_name The name of the shared memory region.
  /// \param byte_size The size of data in bytes.
  /// \param offset The offset in shared memory region. Default value is 0.
  /// \return Error object indicating success or failure of the
  /// request.
  virtual Error SetSharedMemory(
      const std::string& region_name, const size_t byte_size,
      const size_t offset = 0);

 protected:
  InferRequestedOutput(
      const BackendKind kind, const std::string& name,
      const std::string& datatype = "");
  const BackendKind kind_;
  const std::string name_;
  const std::string datatype_;
};

//
// Interface for accessing the processed results.
//
class InferResult {
 public:
  virtual ~InferResult() = default;

  /// Get the id of the request which generated this response.
  /// \param id Returns the request id that generated the result.
  /// \return Error object indicating success or failure.
  virtual Error Id(std::string* id) const = 0;


  /// Returns the status of the request.
  /// \return Error object indicating the success or failure of the
  /// request.
  virtual Error RequestStatus() const = 0;

  /// Returns the raw data of the output.
  /// \return Error object indicating the success or failure of the
  /// request.
  virtual Error RawData(
      const std::string& output_name, std::vector<uint8_t>& buf) const = 0;

  /// Get final response bool for this response.
  /// \return Error object indicating the success or failure.
  virtual Error IsFinalResponse(bool* is_final_response) const
  {
    return Error("InferResult::IsFinalResponse() not implemented");
  };

  /// Get null response bool for this response.
  /// \return Error object indicating the success or failure.
  virtual Error IsNullResponse(bool* is_null_response) const
  {
    return Error("InferResult::IsNullResponse() not implemented");
  };

  /// Returns the response timestamps of the streaming request.
  /// \return Error object indicating the success or failure.
  virtual Error ResponseTimestamps(
      std::vector<std::chrono::time_point<std::chrono::system_clock>>*
          response_timestamps) const
  {
    return Error("InferResult::ResponseTimestamps() not implemented");
  };
};

}}}  // namespace triton::perfanalyzer::clientbackend

namespace cb = triton::perfanalyzer::clientbackend;
