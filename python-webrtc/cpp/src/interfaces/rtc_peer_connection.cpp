//
// Created by Il'ya Semyonov on 1/5/22.
//

#include "rtc_peer_connection.h"

#include <webrtc/p2p/client/basic_port_allocator.h>

#include "peer_connection_factory.h"
#include "create_session_description_observer.h"
#include "set_session_description_observer.h"

namespace python_webrtc {

  RTCPeerConnection::RTCPeerConnection() {
    _factory = PeerConnectionFactory::GetOrCreateDefault();
    _shouldReleaseFactory = true;

//    TODO get from python
    auto configuration = new webrtc::PeerConnectionInterface::RTCConfiguration();

    auto portAllocator = std::unique_ptr<cricket::PortAllocator>(new cricket::BasicPortAllocator(
        _factory->getNetworkManager(),
        _factory->getSocketFactory())
    );

//    TODO get and set port range from configurator.
//    create some struct with min and max uint16_t fields. bind it to python
//    _port_range =  configurator (with additional fields)...port_range

    portAllocator->SetPortRange(0, 65535); // TODO get from config or default

//    TODO switch to CreatePeerConnectionOrError instead, because CreatePeerConnection is deprecated
    _jinglePeerConnection = _factory->factory()->CreatePeerConnection(
        *configuration, // TODO pass rtc configurator
        std::move(portAllocator),
        nullptr,
        this
    );
  }

  RTCPeerConnection::~RTCPeerConnection() {
    _jinglePeerConnection = nullptr;
    // TODO data channels
//    _channels.clear();
    if (_factory) {
      if (_shouldReleaseFactory) {
        PeerConnectionFactory::Release();
      }
      _factory = nullptr;
    }
  }

  void RTCPeerConnection::Init(pybind11::module &m) {
    pybind11::class_<RTCPeerConnection>(m, "RTCPeerConnection")
        .def(pybind11::init<>())
        .def("createOffer", &RTCPeerConnection::CreateOffer)
        .def("createAnswer", &RTCPeerConnection::CreateAnswer)
        .def("setLocalDescription", &RTCPeerConnection::SetLocalDescription)
        .def("setRemoteDescription", &RTCPeerConnection::SetRemoteDescription)
        .def("addTrack", &RTCPeerConnection::AddTrack);
  }

  void RTCPeerConnection::SaveLastSdp(const RTCSessionDescriptionInit &lastSdp) {
    _lastSdp = lastSdp;
  }

  void RTCPeerConnection::CreateOffer(std::function<void(RTCSessionDescription)> &onSuccess) {
    if (!_jinglePeerConnection ||
        _jinglePeerConnection->signaling_state() == webrtc::PeerConnectionInterface::SignalingState::kClosed) {
//      TODO call onFail
//      "Failed to execute 'createOffer' on 'RTCPeerConnection': The RTCPeerConnection's signalingState is 'closed'."
      return;
    }

    auto observer = new rtc::RefCountedObject<CreateSessionDescriptionObserver>(this, onSuccess);

//     TODO bind RTCOfferOptions (voice_activity_detection, iceRestart, offerToReceiveAudio, offerToReceiveVideo)
    auto options = webrtc::PeerConnectionInterface::RTCOfferAnswerOptions();
    options.offer_to_receive_audio = 1;
    options.offer_to_receive_video = 0;

    _jinglePeerConnection->CreateOffer(observer, options);
  }

  void RTCPeerConnection::CreateAnswer(std::function<void(RTCSessionDescription)> &onSuccess) {
    if (!_jinglePeerConnection ||
        _jinglePeerConnection->signaling_state() == webrtc::PeerConnectionInterface::SignalingState::kClosed) {
//      TODO call onFail
//      "Failed to execute 'createAnswer' on 'RTCPeerConnection': The RTCPeerConnection's signalingState is 'closed'."
      return;
    }

    auto observer = new rtc::RefCountedObject<CreateSessionDescriptionObserver>(this, onSuccess);
//       TODO bind RTCAnswerOptions (voice_activity_detection)
    auto options = webrtc::PeerConnectionInterface::RTCOfferAnswerOptions();
    _jinglePeerConnection->CreateAnswer(observer, options);
  }

  void RTCPeerConnection::SetLocalDescription(std::function<void()> &onSuccess, RTCSessionDescription &description) {
//    TODO accept RTCSessionDescriptionInit too
    if (description.getSdp().empty()) {
//      TODO use lastSdp
      _lastSdp.sdp;
    }

    auto *raw_description = static_cast<webrtc::SessionDescriptionInterface *>(description);
    std::unique_ptr<webrtc::SessionDescriptionInterface> raw_description_ptr(raw_description);

    if (!_jinglePeerConnection ||
        _jinglePeerConnection->signaling_state() == webrtc::PeerConnectionInterface::SignalingState::kClosed) {
//      TODO call onFail
//      "Failed to execute 'setLocalDescription' on 'RTCPeerConnection': The RTCPeerConnection's signalingState is 'closed'."
      return;
    }

    auto observer = new rtc::RefCountedObject<SetSessionDescriptionObserver>(onSuccess);
    _jinglePeerConnection->SetLocalDescription(observer, raw_description_ptr.release());
  }

  void RTCPeerConnection::SetRemoteDescription(std::function<void()> &onSuccess, RTCSessionDescription &description) {
//    TODO accept RTCSessionDescriptionInit too

    auto *raw_description = static_cast<webrtc::SessionDescriptionInterface *>(description);
    std::unique_ptr<webrtc::SessionDescriptionInterface> raw_description_ptr(raw_description);

    if (!_jinglePeerConnection ||
        _jinglePeerConnection->signaling_state() == webrtc::PeerConnectionInterface::SignalingState::kClosed) {
//      TODO call onFail
//      "Failed to execute 'setRemoteDescription' on 'RTCPeerConnection': The RTCPeerConnection's signalingState is 'closed'."
      return;
    }

    auto observer = new rtc::RefCountedObject<SetSessionDescriptionObserver>(onSuccess);
    _jinglePeerConnection->SetRemoteDescription(observer, raw_description_ptr.release());
  }

  std::optional<std::reference_wrapper<RTCRtpSender>> RTCPeerConnection::AddTrack(MediaStreamTrack *mediaStreamTrack,
                                                                                  std::optional<std::reference_wrapper<MediaStream>> stream) {
    if (!_jinglePeerConnection) {
      // TODO raise
      // "Cannot addTrack; RTCPeerConnection is closed"
      return {};
    }

    std::vector<std::string> streamIds;
    if (stream != std::nullopt) {
      streamIds.emplace_back(stream->get().stream()->id());
    }

    auto result = _jinglePeerConnection->AddTrack(mediaStreamTrack->track(), streamIds);
    if (!result.ok()) {
      // TODO raise
      // result.error() // RTCError
      return {};
    }

    auto rtpSender = RTCRtpSender(_factory, result.value());
    return rtpSender;
  }

  void RTCPeerConnection::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) {
//    TODO call python callback
    if (new_state == webrtc::PeerConnectionInterface::kClosed) {
//      TODO stop
    }
  }

  void RTCPeerConnection::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) {

  }

  void RTCPeerConnection::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) {

  }

  void RTCPeerConnection::OnIceCandidate(const webrtc::IceCandidateInterface *candidate) {

  }

  void RTCPeerConnection::OnIceCandidateError(const std::string &host_candidate, const std::string &url, int error_code,
                                              const std::string &error_text) {

  }

  void RTCPeerConnection::OnRenegotiationNeeded() {

  }

  void RTCPeerConnection::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) {

  }

  void RTCPeerConnection::OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) {

  }

  void RTCPeerConnection::OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) {

  }

  void RTCPeerConnection::OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
                                     const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>> &streams) {

  }

  void RTCPeerConnection::OnTrack(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) {

  }

} // namespace python_webrtc
