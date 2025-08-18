// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "RibbonSupport.h"
#include "MainFrame.h"
#include "resource.h"
#include <UIRibbonPropertyHelpers.h>

namespace fusion {
namespace debugviewpp {

RibbonSupport::RibbonSupport(CMainFrame* pMainFrame) :
    m_pMainFrame(pMainFrame),
    m_cRef(1),
    m_bInitialized(false),
    m_ribbonHeight(0)
{
}

RibbonSupport::~RibbonSupport()
{
    Destroy();
}

HRESULT RibbonSupport::Initialize(HWND hWnd)
{
    if (m_bInitialized)
        return S_OK;

    // Check if ribbon is available on this system
    HRESULT hr = ::CoCreateInstance(CLSID_UIRibbonFramework, NULL, CLSCTX_INPROC_SERVER, 
        IID_PPV_ARGS(&m_pFramework));
    
    if (FAILED(hr))
    {
        // Ribbon not available (pre-Windows 7)
        return hr;
    }

    // Initialize the ribbon framework
    hr = m_pFramework->Initialize(hWnd, this);
    if (FAILED(hr))
    {
        m_pFramework.Release();
        return hr;
    }

    // Load the ribbon resource
    // Note: IDR_RIBBON will be generated from DebugViewRibbon.xml by UICC compiler
    hr = m_pFramework->LoadUI(GetModuleHandle(NULL), L"APPLICATION_RIBBON");
    if (FAILED(hr))
    {
        m_pFramework->Destroy();
        m_pFramework.Release();
        return hr;
    }

    m_bInitialized = true;
    return S_OK;
}

void RibbonSupport::Destroy()
{
    if (m_pFramework)
    {
        m_pFramework->Destroy();
        m_pFramework.Release();
    }
    m_pRibbon.Release();
    m_bInitialized = false;
}

bool RibbonSupport::IsAvailable() const
{
    return m_bInitialized && m_pFramework != nullptr;
}

bool RibbonSupport::IsVisible() const
{
    if (!IsAvailable() || !m_pFramework)
        return false;

    PROPVARIANT pv;
    PropVariantInit(&pv);
    HRESULT hr = m_pFramework->GetUICommandProperty(0, UI_PKEY_Viewable, &pv);
    if (SUCCEEDED(hr))
    {
        BOOL bVisible = FALSE;
        UIPropertyToBoolean(UI_PKEY_Viewable, pv, &bVisible);
        PropVariantClear(&pv);
        return bVisible == TRUE;
    }
    return false;
}

void RibbonSupport::Show(bool show)
{
    if (!IsAvailable() || !m_pFramework)
        return;

    if (show)
    {
        // Show the ribbon
        m_pFramework->SetModes(UI_MAKEAPPMODE(0));
        
        // Force framework to update the view
        if (m_pRibbon)
        {
            PROPVARIANT pv;
            PropVariantInit(&pv);
            UIInitPropertyFromBoolean(UI_PKEY_Viewable, TRUE, &pv);
            m_pFramework->SetUICommandProperty(0, UI_PKEY_Viewable, pv);
            PropVariantClear(&pv);
        }
    }
    else
    {
        // Hide the ribbon
        if (m_pRibbon)
        {
            PROPVARIANT pv;
            PropVariantInit(&pv);
            UIInitPropertyFromBoolean(UI_PKEY_Viewable, FALSE, &pv);
            m_pFramework->SetUICommandProperty(0, UI_PKEY_Viewable, pv);
            PropVariantClear(&pv);
        }
        m_pFramework->SetModes(UI_MAKEAPPMODE(1));
    }
}

UINT RibbonSupport::GetHeight() const
{
    if (!IsAvailable() || !m_pRibbon)
        return 0;

    UINT height = 0;
    m_pRibbon->GetHeight(&height);
    return height;
}

// IUnknown methods
STDMETHODIMP RibbonSupport::QueryInterface(REFIID riid, void** ppvObject)
{
    if (riid == __uuidof(IUnknown))
    {
        *ppvObject = static_cast<IUnknown*>(static_cast<IUIApplication*>(this));
    }
    else if (riid == __uuidof(IUIApplication))
    {
        *ppvObject = static_cast<IUIApplication*>(this);
    }
    else if (riid == __uuidof(IUICommandHandler))
    {
        *ppvObject = static_cast<IUICommandHandler*>(this);
    }
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) RibbonSupport::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) RibbonSupport::Release()
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0)
    {
        delete this;
    }
    return cRef;
}

// IUIApplication methods
STDMETHODIMP RibbonSupport::OnViewChanged(UINT32 viewId, UI_VIEWTYPE typeID, IUnknown* view, 
    UI_VIEWVERB verb, INT32 uReasonCode)
{
    // Handle ribbon view state changes
    if (typeID == UI_VIEWTYPE_RIBBON)
    {
        switch (verb)
        {
        case UI_VIEWVERB_CREATE:
            // Ribbon is being created
            if (view)
            {
                view->QueryInterface(IID_PPV_ARGS(&m_pRibbon));
            }
            break;

        case UI_VIEWVERB_SIZE:
            // Ribbon size changed
            if (m_pRibbon)
            {
                m_pRibbon->GetHeight(&m_ribbonHeight);
                // Notify main frame to adjust layout if needed
                if (m_pMainFrame)
                {
                    m_pMainFrame->UpdateLayout();
                }
            }
            break;

        case UI_VIEWVERB_DESTROY:
            // Ribbon is being destroyed
            m_pRibbon.Release();
            m_ribbonHeight = 0;
            break;
        }
    }
    return S_OK;
}

STDMETHODIMP RibbonSupport::OnCreateUICommand(UINT32 commandId, UI_COMMANDTYPE typeID, 
    IUICommandHandler** commandHandler)
{
    // Return this object as the command handler for all commands
    *commandHandler = static_cast<IUICommandHandler*>(this);
    (*commandHandler)->AddRef();
    return S_OK;
}

STDMETHODIMP RibbonSupport::OnDestroyUICommand(UINT32 commandId, UI_COMMANDTYPE typeID, 
    IUICommandHandler* commandHandler)
{
    // Nothing to do
    return S_OK;
}

// IUICommandHandler methods
STDMETHODIMP RibbonSupport::Execute(UINT32 commandId, UI_EXECUTIONVERB verb, const PROPERTYKEY* key, 
    const PROPVARIANT* currentValue, IUISimplePropertySet* commandExecutionProperties)
{
    if (verb != UI_EXECUTIONVERB_EXECUTE)
        return S_OK;

    // Route commands to the main frame's existing command handlers
    if (m_pMainFrame)
    {
        // Send as WM_COMMAND to use existing handlers
        m_pMainFrame->PostMessage(WM_COMMAND, MAKEWPARAM(commandId, 0), 0);
    }

    return S_OK;
}

STDMETHODIMP RibbonSupport::UpdateProperty(UINT32 commandId, REFPROPERTYKEY key, 
    const PROPVARIANT* currentValue, PROPVARIANT* newValue)
{
    // Handle property updates (labels, enabled state, etc.)
    // For now, just return S_FALSE to use defaults
    return S_FALSE;
}

} // namespace debugviewpp
} // namespace fusion