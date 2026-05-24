#include "ApplicationDX12.hpp"
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>

namespace elementalEngine {
ApplicationDX12::ApplicationDX12() { initDX12(); }
ApplicationDX12::~ApplicationDX12() {
  // TODO cleaning up
}

void ApplicationDX12::run() {
  while (!window.shouldClose()) {
    glfwPollEvents();
  }
}

void ApplicationDX12::initDX12() {
  enableDebugLayer();
  createFactory();
}

// debug layout
void ApplicationDX12::enableDebugLayer() {

  if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
    debugController->EnableDebugLayer();

    debugController->SetEnableGPUBasedValidation(true);
    std::cout << "D3D12 Debug Layer and GPU-Based Validation Enabled.\n";
  } else {
    std::cerr << "Direct3D Debug Device is not available. Are "
                 "Graphics Tools are installed?\n";
  }
}

// instance
void ApplicationDX12::createFactory() {
  UINT factoryFlags = 0;

  if (debugController != nullptr) {
    factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
  }

  if (FAILED(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&factory)))) {
    throw std::runtime_error("Failed to create DXGI Factory!");
  }
}

} // namespace elementalEngine
