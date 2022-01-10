//
// Created by Il'ya Semyonov on 1/5/22.
//

#include "interfaces.h"

#include "peer_connection_factory.h"
#include "rtc_peer_connection.h"
#include "media_stream_track.h"
#include "media_stream.h"

namespace python_webrtc {

  void Interfaces::Init(pybind11::module &m) {
    PeerConnectionFactory::Init(m);
    RTCPeerConnection::Init(m);
    MediaStreamTrack::Init(m);
    MediaStream::Init(m);
  }
}
