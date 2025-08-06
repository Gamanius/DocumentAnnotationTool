#include "TSFHandler.h"

#include "textstor.h"
#include "msctf.h"

#include <wrl/client.h>

using namespace Microsoft::WRL;

class MyTextStore : public ITextStoreACP {
public:
	ULONG refCount;
	std::wstring textBuffer;

    HRESULT STDMETHODCALLTYPE QueryInterface(
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) {
        *ppvObject = nullptr;
        if ((IID_IUnknown == riid) || (IID_ITextStoreACP == riid)) {
            *ppvObject = static_cast<ITextStoreACP*>(this);
        }
        if (*ppvObject) {
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef(void) {
        return ++refCount;
    }

    ULONG STDMETHODCALLTYPE Release(void) {
        return --refCount;
    }

    HRESULT STDMETHODCALLTYPE AdviseSink(
        /* [in] */ __RPC__in REFIID riid,
        /* [iid_is][in] */ __RPC__in_opt IUnknown* punk,
        /* [in] */ DWORD dwMask) override { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE UnadviseSink(
        /* [in] */ __RPC__in_opt IUnknown* punk) override { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE RequestLock(
        /* [in] */ DWORD dwLockFlags,
        /* [out] */ __RPC__out HRESULT* phrSession) override { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE GetStatus(
        /* [out] */ __RPC__out TS_STATUS* pdcs) override { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE QueryInsert(
        /* [in] */ LONG acpTestStart,
        /* [in] */ LONG acpTestEnd,
        /* [in] */ ULONG cch,
        /* [out] */ __RPC__out LONG* pacpResultStart,
        /* [out] */ __RPC__out LONG* pacpResultEnd) override { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE GetSelection(
        /* [in] */ ULONG ulIndex,
        /* [in] */ ULONG ulCount,
        /* [length_is][size_is][out] */ __RPC__out_ecount_part(ulCount, *pcFetched) TS_SELECTION_ACP* pSelection,
        /* [out] */ __RPC__out ULONG* pcFetched) override { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE SetSelection(
        /* [in] */ ULONG ulCount,
        /* [size_is][in] */ __RPC__in_ecount_full(ulCount) const TS_SELECTION_ACP* pSelection) override { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE GetText(
        /* [in] */ LONG acpStart,
        /* [in] */ LONG acpEnd,
        /* [length_is][size_is][out] */ __RPC__out_ecount_part(cchPlainReq, *pcchPlainRet) WCHAR* pchPlain,
        /* [in] */ ULONG cchPlainReq,
        /* [out] */ __RPC__out ULONG* pcchPlainRet,
        /* [length_is][size_is][out] */ __RPC__out_ecount_part(cRunInfoReq, *pcRunInfoRet) TS_RUNINFO* prgRunInfo,
        /* [in] */ ULONG cRunInfoReq,
        /* [out] */ __RPC__out ULONG* pcRunInfoRet,
        /* [out] */ __RPC__out LONG* pacpNext) override { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE SetText(
        /* [in] */ DWORD dwFlags,
        /* [in] */ LONG acpStart,
        /* [in] */ LONG acpEnd,
        /* [size_is][in] */ __RPC__in_ecount_full(cch) const WCHAR* pchText,
        /* [in] */ ULONG cch,
        /* [out] */ __RPC__out TS_TEXTCHANGE* pChange) override { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE GetFormattedText(
        /* [in] */ LONG acpStart,
        /* [in] */ LONG acpEnd,
        /* [out] */ __RPC__deref_out_opt IDataObject** ppDataObject) override { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE GetEmbedded(
        /* [in] */ LONG acpPos,
        /* [in] */ __RPC__in REFGUID rguidService,
        /* [in] */ __RPC__in REFIID riid,
        /* [iid_is][out] */ __RPC__deref_out_opt IUnknown** ppunk) override { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE QueryInsertEmbedded(
        /* [in] */ __RPC__in const GUID* pguidService,
        /* [in] */ __RPC__in const FORMATETC* pFormatEtc,
        /* [out] */ __RPC__out BOOL* pfInsertable) override { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE InsertEmbedded(
        /* [in] */ DWORD dwFlags,
        /* [in] */ LONG acpStart,
        /* [in] */ LONG acpEnd,
        /* [in] */ __RPC__in_opt IDataObject* pDataObject,
        /* [out] */ __RPC__out TS_TEXTCHANGE* pChange) override { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE InsertTextAtSelection(
        /* [in] */ DWORD dwFlags,
        /* [size_is][in] */ __RPC__in_ecount_full(cch) const WCHAR* pchText,
        /* [in] */ ULONG cch,
        /* [out] */ __RPC__out LONG* pacpStart,
        /* [out] */ __RPC__out LONG* pacpEnd,
        /* [out] */ __RPC__out TS_TEXTCHANGE* pChange) override { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE InsertEmbeddedAtSelection(
        /* [in] */ DWORD dwFlags,
        /* [in] */ __RPC__in_opt IDataObject* pDataObject,
        /* [out] */ __RPC__out LONG* pacpStart,
        /* [out] */ __RPC__out LONG* pacpEnd,
        /* [out] */ __RPC__out TS_TEXTCHANGE* pChange) override { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE RequestSupportedAttrs(
        /* [in] */ DWORD dwFlags,
        /* [in] */ ULONG cFilterAttrs,
        /* [unique][size_is][in] */ __RPC__in_ecount_full_opt(cFilterAttrs) const TS_ATTRID* paFilterAttrs) override { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE RequestAttrsAtPosition(
        /* [in] */ LONG acpPos,
        /* [in] */ ULONG cFilterAttrs,
        /* [unique][size_is][in] */ __RPC__in_ecount_full_opt(cFilterAttrs) const TS_ATTRID* paFilterAttrs,
        /* [in] */ DWORD dwFlags) override { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE RequestAttrsTransitioningAtPosition(
        /* [in] */ LONG acpPos,
        /* [in] */ ULONG cFilterAttrs,
        /* [unique][size_is][in] */ __RPC__in_ecount_full_opt(cFilterAttrs) const TS_ATTRID* paFilterAttrs,
        /* [in] */ DWORD dwFlags) override { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE FindNextAttrTransition(
        /* [in] */ LONG acpStart,
        /* [in] */ LONG acpHalt,
        /* [in] */ ULONG cFilterAttrs,
        /* [unique][size_is][in] */ __RPC__in_ecount_full_opt(cFilterAttrs) const TS_ATTRID* paFilterAttrs,
        /* [in] */ DWORD dwFlags,
        /* [out] */ __RPC__out LONG* pacpNext,
        /* [out] */ __RPC__out BOOL* pfFound,
        /* [out] */ __RPC__out LONG* plFoundOffset) override { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE RetrieveRequestedAttrs(
        /* [in] */ ULONG ulCount,
        /* [length_is][size_is][out] */ __RPC__out_ecount_part(ulCount, *pcFetched) TS_ATTRVAL* paAttrVals,
        /* [out] */ __RPC__out ULONG* pcFetched) override { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE GetEndACP(
        /* [out] */ __RPC__out LONG* pacp) override { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE GetActiveView(
        /* [out] */ __RPC__out TsViewCookie* pvcView) override { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE GetACPFromPoint(
        /* [in] */ TsViewCookie vcView,
        /* [in] */ __RPC__in const POINT* ptScreen,
        /* [in] */ DWORD dwFlags,
        /* [out] */ __RPC__out LONG* pacp) override { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE GetTextExt(
        /* [in] */ TsViewCookie vcView,
        /* [in] */ LONG acpStart,
        /* [in] */ LONG acpEnd,
        /* [out] */ __RPC__out RECT* prc,
        /* [out] */ __RPC__out BOOL* pfClipped) override { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE GetScreenExt(
        /* [in] */ TsViewCookie vcView,
        /* [out] */ __RPC__out RECT* prc) override { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE GetWnd(
        /* [in] */ TsViewCookie vcView,
        /* [out] */ __RPC__deref_out_opt HWND* phwnd) override { return E_NOTIMPL; }
};


struct DocantoWin::TSFHandler::impl {
	TfClientId id;
    TfEditCookie cookie;

	ComPtr<ITfThreadMgr> threadManager = nullptr;
	ComPtr<ITfDocumentMgr> documentManager = nullptr;
    ComPtr<ITfContext> context = nullptr;
    ComPtr<MyTextStore> store = nullptr;
};

DocantoWin::TSFHandler::TSFHandler(std::shared_ptr<Window> w) : pimpl(std::make_unique<impl>()) {
	auto res = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	res = CoCreateInstance(CLSID_TF_ThreadMgr, NULL, CLSCTX_INPROC_SERVER, IID_ITfThreadMgr, (void**)pimpl->threadManager.GetAddressOf());
	res = pimpl->threadManager->Activate(&pimpl->id);
	res = pimpl->threadManager->CreateDocumentMgr(pimpl->documentManager.GetAddressOf());

    pimpl->store.Attach(new MyTextStore());

    pimpl->documentManager->CreateContext(pimpl->id, 0, pimpl->store.Get(), pimpl->context.GetAddressOf(), &pimpl->cookie);
}

DocantoWin::TSFHandler::~TSFHandler() {

}
