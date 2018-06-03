#include "cbase.h"

#include "hud.h"
#include "hudelement.h"
#include "c_resourcegatherer_player.h"
#include "c_playerresource.h"
#include "vgui_entitypanel.h"
#include "iclientmode.h"
#include "vgui/ILocalize.h"
#include "resourcegatherer_gamerules.h"
#include "c_team.h"
#include <vgui_controls/AnimationController.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CResouceGathererResouceCounterHud : public vgui::Panel, public CHudElement
{
    DECLARE_CLASS_SIMPLE( CResouceGathererResouceCounterHud, vgui::Panel );

public:
    CResouceGathererResouceCounterHud(const char* pElementName);
    void Reset();

    virtual void PerformLayout();

protected:
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnThink();

private:
    vgui::HFont m_hFont;

	CPanelAnimationVarAliasType(int, m_iTextX, "text_xpos", "8", "proportional_int");
	CPanelAnimationVarAliasType(int, m_iTextY, "text_ypos", "8", "proportional_int");

    vgui::Label* m_pBiologicalLabel;
};

DECLARE_HUDELEMENT( CResouceGathererResouceCounterHud );

CResouceGathererResouceCounterHud::CResouceGathererResouceCounterHud(const char* pElementName) : BaseClass(NULL, "ResourceCounter"), CHudElement(pElementName)
{
    vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
	SetVisible( false );
	SetAlpha( 255 );

    m_pBiologicalLabel = new vgui::Label( this, "BiologicalLabel", "Biological: 0" );
    m_pBiologicalLabel->SetPaintBackgroundEnabled( false );
	m_pBiologicalLabel->SetPaintBorderEnabled( false );
	m_pBiologicalLabel->SizeToContents();
	m_pBiologicalLabel->SetContentAlignment( vgui::Label::a_west );
	m_pBiologicalLabel->SetFgColor( GetFgColor() );
}

void CResouceGathererResouceCounterHud::Reset()
{
}

void CResouceGathererResouceCounterHud::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetFgColor( Color(0,0,0,0) );	//GetSchemeColor("RoundStateFg", pScheme) );
	m_hFont = pScheme->GetFont( "Default", true );

	//m_pBackground->SetBgColor( GetSchemeColor("BgColor", pScheme) );
	//m_pBackground->SetPaintBackgroundType( 2 );

	SetAlpha( 255 );
	SetBgColor( Color( 0, 0, 0, 0 ) );
	SetPaintBackgroundType( 0 );
}

void CResouceGathererResouceCounterHud::PerformLayout()
{
	BaseClass::PerformLayout();

	int wide, tall;
	GetSize( wide, tall );

	// find the widest line
	int labelWide = m_pBiologicalLabel->GetWide();

	// find the total height
	int fontTall = vgui::surface()->GetFontTall( m_hFont );
	int labelTall = fontTall;

	labelWide += m_iTextX*2;
	labelTall += m_iTextY*2;

	//m_pBackground->SetBounds( 0, 0, labelWide, labelTall );

	int xOffset = (labelWide - m_pBiologicalLabel->GetWide())/2;
	m_pBiologicalLabel->SetPos( 0 + xOffset, 0 + m_iTextY );
}

void CResouceGathererResouceCounterHud::OnThink()
{
    //SetVisible( false );

    char szNum[6];
    Q_snprintf( szNum, sizeof(szNum), "%i", ResourceGathererRules()->GetBiologicalResourceCount() );
    wchar_t wNum[4];
    g_pVGuiLocalize->ConvertANSIToUnicode(szNum, wNum, sizeof(wNum));

    wchar_t bioLabel[1024];
    g_pVGuiLocalize->ConstructString( bioLabel, sizeof(bioLabel), g_pVGuiLocalize->Find("#ResourceGatherer_ResourceCounter_Biological"), 1, wNum );

    m_pBiologicalLabel->SetText( bioLabel );
    m_pBiologicalLabel->SetVisible( true );

    m_pBiologicalLabel->SizeToContents();

    InvalidateLayout();
}