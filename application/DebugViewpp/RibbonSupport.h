// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "atleverything.h"
#include <UIRibbon.h>
#include <UIRibbonPropertyHelpers.h>

namespace fusion {
namespace debugviewpp {

class CMainFrame;

class RibbonSupport : 
    public IUICommandHandler,
    public IUIApplication
{
public:
    RibbonSupport(CMainFrame* pMainFrame);
    virtual ~RibbonSupport();

    HRESULT Initialize(HWND hWnd);
    void Destroy();
    bool IsAvailable() const;
    bool IsVisible() const;
    void Show(bool show);
    UINT GetHeight() const;

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void** ppvObject);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IUIApplication methods
    STDMETHOD(OnViewChanged)(UINT32 viewId, UI_VIEWTYPE typeID, IUnknown* view, UI_VIEWVERB verb, INT32 uReasonCode);
    STDMETHOD(OnCreateUICommand)(UINT32 commandId, UI_COMMANDTYPE typeID, IUICommandHandler** commandHandler);
    STDMETHOD(OnDestroyUICommand)(UINT32 commandId, UI_COMMANDTYPE typeID, IUICommandHandler* commandHandler);

    // IUICommandHandler methods
    STDMETHOD(Execute)(UINT32 commandId, UI_EXECUTIONVERB verb, const PROPERTYKEY* key, const PROPVARIANT* currentValue, IUISimplePropertySet* commandExecutionProperties);
    STDMETHOD(UpdateProperty)(UINT32 commandId, REFPROPERTYKEY key, const PROPVARIANT* currentValue, PROPVARIANT* newValue);

private:
    CMainFrame* m_pMainFrame;
    CComPtr<IUIFramework> m_pFramework;
    CComPtr<IUIRibbon> m_pRibbon;
    LONG m_cRef;
    bool m_bInitialized;
    UINT m_ribbonHeight;
};

} // namespace debugviewpp
} // namespace fusion