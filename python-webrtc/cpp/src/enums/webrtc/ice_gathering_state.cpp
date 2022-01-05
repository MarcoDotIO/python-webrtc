//
// Created by Il'ya Semyonov on 1/5/22.
//

#include <api/peer_connection_interface.h>

#include "ice_gathering_state.h"

namespace python_webrtc {

  void IceGatheringState::Init(pybind11::module &m) {
    pybind11::enum_<webrtc::PeerConnectionInterface::IceGatheringState>(m,"RTCIceGatheringState")
        .value("new", webrtc::PeerConnectionInterface::IceGatheringState::kIceGatheringNew)
        .value("gathering", webrtc::PeerConnectionInterface::IceGatheringState::kIceGatheringGathering)
        .value("complete", webrtc::PeerConnectionInterface::IceGatheringState::kIceGatheringComplete)
        .export_values();
  }

}
