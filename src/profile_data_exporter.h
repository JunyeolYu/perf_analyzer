// Copyright 2023-2025, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
#include <sys/types.h>

#include "client_backend/client_backend.h"
#include "profile_data_collector.h"

namespace triton { namespace perfanalyzer {

#ifndef DOCTEST_CONFIG_DISABLE
class NaggyMockProfileDataExporter;
#endif

/// Exports profile data.
class ProfileDataExporter {
 public:
  static cb::Error Create(std::shared_ptr<ProfileDataExporter>* exporter);
  ~ProfileDataExporter() = default;

  /// Export profile data to json file
  /// @param raw_experiments All of the raw data for the experiments run by perf
  /// analyzer
  /// @param raw_version String containing the version number for the json
  /// output
  /// @param file_path File path to export profile data to.
  /// @param service_kind Service that Perf Analyzer generates load for.
  /// @param endpoint Endpoint to send the requests.
  void Export(
      const std::vector<ProfileDataCollector::Experiment>& raw_experiments,
      std::string& raw_version, std::string& file_path,
      cb::BackendKind& service_kind, std::string& endpoint);
  void set_simple() { simple = true; }

 private:
  ProfileDataExporter() = default;
  /// Convert the raw data collected to json output
  /// @param raw_experiments All of the raw data for the experiments run by perf
  /// analyzer
  /// @param raw_version String containing the version number for the json
  /// output
  /// @param service_kind Service that Perf Analyzer generates load for.
  /// @param endpoint Endpoint to send the requests.
  virtual void ConvertToJson(
      const std::vector<ProfileDataCollector::Experiment>& raw_experiments,
      std::string& raw_version, cb::BackendKind& service_kind,
      std::string& endpoint);
  virtual void OutputToFile(std::string& file_path);
  virtual void AddExperiment(
      rapidjson::Value& entry, rapidjson::Value& experiment,
      const ProfileDataCollector::Experiment& raw_experiment);
  void AddRequests(
      rapidjson::Value& entry, rapidjson::Value& requests,
      const ProfileDataCollector::Experiment& raw_experiment);
  void SetValueToJSON(
      rapidjson::Value& json, const size_t index,
      const std::vector<uint8_t>& buf, const std::string& data_type);
  void AddDataToJSON(
      rapidjson::Value& json, const std::vector<uint8_t>& buf,
      const std::string& data_type);
  void AddRequestInputs(
      rapidjson::Value& inputs_json,
      const std::vector<RequestRecord::RequestInput>& inputs);
  void AddResponseTimestamps(
      rapidjson::Value& timestamps_json,
      const std::vector<std::chrono::time_point<std::chrono::system_clock>>&
          timestamps);
  void AddResponseOutputs(
      rapidjson::Value& outputs_json,
      const std::vector<RequestRecord::ResponseOutput>& outputs);
  void AddWindowBoundaries(
      rapidjson::Value& entry, rapidjson::Value& window_boundaries,
      const ProfileDataCollector::Experiment& raw_experiment);
  void AddVersion(std::string& raw_version);
  void AddServiceKind(cb::BackendKind& service_kind);
  void AddEndpoint(std::string& endpoint);
  void ClearDocument();

  rapidjson::Document document_{};

  // for simplified profile log
  bool simple = false;
  u_int64_t start_time;

#ifndef DOCTEST_CONFIG_DISABLE
  friend NaggyMockProfileDataExporter;
#endif
};
}}  // namespace triton::perfanalyzer
