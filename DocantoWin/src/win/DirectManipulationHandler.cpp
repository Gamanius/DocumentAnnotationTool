#include "DirectManipulationHandler.h"

IDirectManipulationManager* DocantoWin::DirectManipulationHandler::m_manager = nullptr;

class DMEventHandler : public IDirectManipulationViewportEventHandler,
					   public IDirectManipulationInteractionEventHandler {
private:
	long m_refCount; // Reference count for IUnknown
public:
	DMEventHandler() : m_refCount(1) { // Initialize ref count to 1 on creation
		// CoIncrementMTAUsage() or CoInitializeEx should be handled by the caller
	}

	// IUnknown methods
	STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject) override {
		if (!ppvObject) return E_POINTER;
		*ppvObject = nullptr;

		if (IsEqualIID(riid, IID_IUnknown) ||
			IsEqualIID(riid, IID_IDirectManipulationViewportEventHandler)) {
			*ppvObject = static_cast<IDirectManipulationViewportEventHandler*>(this);
			AddRef();
			return S_OK;
		}
		if (IsEqualIID(riid, IID_IDirectManipulationInteractionEventHandler)) {
			*ppvObject = static_cast<IDirectManipulationInteractionEventHandler*>(this);
			AddRef();
			return S_OK;
		}
		return E_NOINTERFACE;
	}

	STDMETHODIMP_(ULONG) AddRef() override {
		return InterlockedIncrement(&m_refCount);
	}

	STDMETHODIMP_(ULONG) Release() override {
		ULONG ulCount = InterlockedDecrement(&m_refCount);
		if (ulCount == 0) {
			delete this; // Self-delete when ref count reaches 0
		}
		return ulCount;
	}
	HRESULT OnViewportStatusChanged(
		IDirectManipulationViewport* viewport, DIRECTMANIPULATION_STATUS current,
		DIRECTMANIPULATION_STATUS previous) override {
		return S_OK;
	}

	HRESULT OnViewportUpdated(IDirectManipulationViewport* viewport) override {
		return S_OK;
	}

	HRESULT OnContentUpdated(IDirectManipulationViewport* viewport,
		IDirectManipulationContent* content) override {
		return S_OK;
	}

	HRESULT OnInteraction(IDirectManipulationViewport2* viewport,
		DIRECTMANIPULATION_INTERACTION_TYPE interaction) override {
		return S_OK;
	}
};

std::unique_ptr<DMEventHandler> g_eventhandler;

DocantoWin::DirectManipulationHandler::DirectManipulationHandler(HWND h) : m_attached_window(h) {
	if (m_manager == nullptr) {
		CoCreateInstance(
			CLSID_DirectManipulationManager,    // CLSID of the object to create
			NULL,                               // Outer unknown (for aggregation, not common here)
			CLSCTX_INPROC_SERVER,               // In-process server (DLL)
			IID_IDirectManipulationManager,     // Interface ID of the desired interface
			(void**)&m_manager                       // Pointer to receive the interface pointer
		);
	}
	else {
		m_manager->AddRef();
	}

	m_manager->GetUpdateManager(IID_IDirectManipulationUpdateManager, (void**)&m_updatemanager);

	m_manager->CreateViewport(nullptr, h, IID_IDirectManipulationViewport, (void**)&m_viewport);

	DIRECTMANIPULATION_CONFIGURATION configuration =
		DIRECTMANIPULATION_CONFIGURATION_INTERACTION |
		DIRECTMANIPULATION_CONFIGURATION_TRANSLATION_X |
		DIRECTMANIPULATION_CONFIGURATION_TRANSLATION_Y |
		DIRECTMANIPULATION_CONFIGURATION_TRANSLATION_INERTIA |
		DIRECTMANIPULATION_CONFIGURATION_RAILS_X |
		DIRECTMANIPULATION_CONFIGURATION_RAILS_Y |
		DIRECTMANIPULATION_CONFIGURATION_SCALING;

	m_viewport->ActivateConfiguration(configuration);
	m_viewport->SetViewportOptions(DIRECTMANIPULATION_VIEWPORT_OPTIONS_MANUALUPDATE);

	g_eventhandler = std::make_unique<DMEventHandler>();

	m_viewport->AddEventHandler(h, g_eventhandler.get(), &m_eventhandlercookie);
	RECT r;
	GetClientRect(h, &r);
	m_viewport->SetViewportRect(&r);

	m_manager->Activate(h);
	m_viewport->Enable();
	m_updatemanager->Update(nullptr);
}

DocantoWin::DirectManipulationHandler::~DirectManipulationHandler() {

}
