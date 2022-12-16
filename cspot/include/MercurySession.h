#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "BellTask.h"
#include "Logger.h"
#include "NanoPBHelper.h"
#include "Packet.h"
#include "Queue.h"
#include "Session.h"
#include "TimeProvider.h"
#include "Utils.h"
#include "protobuf/mercury.pb.h"

namespace cspot {
class MercurySession : public bell::Task, public cspot::Session {
 public:
  MercurySession(std::shared_ptr<cspot::TimeProvider> timeProvider);
  ~MercurySession();
  typedef std::vector<std::vector<uint8_t>> DataParts;

  struct Response {
    Header mercuryHeader;
    uint8_t flags;
    DataParts parts;
    uint64_t sequenceId;
  };

  typedef std::function<void(Response&)> ResponseCallback;
  typedef std::function<void(bool, const std::vector<uint8_t>&)> AudioKeyCallback;

  enum class RequestType : uint8_t {
    SUB = 0xb3,
    UNSUB = 0xb4,
    SUBRES = 0xb5,
    SEND = 0xb2,
    GET = 0xFF,  // Shitty workaround, it's value is actually same as SEND
    PING = 0x04,
    PONG_ACK = 0x4a,
    AUDIO_CHUNK_REQUEST_COMMAND = 0x08,
    AUDIO_CHUNK_SUCCESS_RESPONSE = 0x09,
    AUDIO_CHUNK_FAILURE_RESPONSE = 0x0A,
    AUDIO_KEY_REQUEST_COMMAND = 0x0C,
    AUDIO_KEY_SUCCESS_RESPONSE = 0x0D,
    AUDIO_KEY_FAILURE_RESPONSE = 0x0E,
    COUNTRY_CODE_RESPONSE = 0x1B,
  };

  std::unordered_map<RequestType, std::string> RequestTypeMap = {
      {RequestType::GET, "GET"},
      {RequestType::SEND, "SEND"},
      {RequestType::SUB, "SUB"},
      {RequestType::UNSUB, "UNSUB"},
  };

  void handlePacket();

  uint64_t executeSubscription(RequestType type, const std::string& uri,
                               ResponseCallback callback,
                               ResponseCallback subscription, DataParts& parts);
  uint64_t executeSubscription(RequestType type, const std::string& uri,
                               ResponseCallback callback,
                               ResponseCallback subscription) {
    DataParts parts = {};
    return this->executeSubscription(type, uri, callback, subscription, parts);
  }

  uint64_t execute(RequestType type, const std::string& uri,
                   ResponseCallback callback) {
    return this->executeSubscription(type, uri, callback, nullptr);
  }

  uint64_t execute(RequestType type, const std::string& uri,
                   ResponseCallback callback, DataParts& parts) {
    return this->executeSubscription(type, uri, callback, nullptr, parts);
  }

  void requestAudioKey(const std::vector<uint8_t>& trackId,
                       const std::vector<uint8_t>& fileId,
                       AudioKeyCallback audioCallback);

 private:
  std::shared_ptr<cspot::TimeProvider> timeProvider;
  Header tempMercuryHeader = {};

  bell::Queue<cspot::Packet> packetQueue;

  void runTask() override;

  std::unordered_map<uint64_t, ResponseCallback> callbacks;
  std::unordered_map<std::string, ResponseCallback> subscriptions;
  AudioKeyCallback audioKeyCallback;

  uint64_t sequenceId = 1;
  uint32_t audioKeySequence = 1;

  unsigned long long timestampDiff;
  unsigned long long lastPingTimestamp = -1;

  Response decodeResponse(const std::vector<uint8_t>& data);
};
}  // namespace cspot