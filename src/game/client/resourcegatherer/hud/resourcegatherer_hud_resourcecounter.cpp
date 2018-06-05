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
	vgui::Label* m_pMechanicalLabel;
	vgui::Label* m_pEnergyLabel;

	vgui::Label* m_pBackground;
};

DECLARE_HUDELEMENT( CResouceGathererResouceCounterHud );

CResouceGathererResouceCounterHud::CResouceGathererResouceCounterHud(const char* pElementName) : BaseClass(NULL, "ResourceCounter"), CHudElement(pElementName)
{
    vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
	SetVisible( false );
	SetAlpha( 255 );

	m_pBackground = new vgui::Label(this, "Background", "");

    m_pBiologicalLabel = new vgui::Label( this, "BiologicalLabel", "Biological: 0" );
    m_pBiologicalLabel->SetPaintBackgroundEnabled( false );
	m_pBiologicalLabel->SetPaintBorderEnabled( false );
	m_pBiologicalLabel->SizeToContents();
	m_pBiologicalLabel->SetContentAlignment( vgui::Label::a_west );
	m_pBiologicalLabel->SetFgColor( GetFgColor() );

	m_pMechanicalLabel = new vgui::Label(this, "MechanicalLabel", "Mechanical: 0");
	m_pMechanicalLabel->SetPaintBackgroundEnabled(false);
	m_pMechanicalLabel->SetPaintBorderEnabled(false);
	m_pMechanicalLabel->SizeToContents();
	m_pMechanicalLabel->SetContentAlignment(vgui::Label::a_west);
	m_pMechanicalLabel->SetFgColor(GetFgColor());

	m_pEnergyLabel = new vgui::Label(this, "EnergyLabel", "Energy: 0");
	m_pEnergyLabel->SetPaintBackgroundEnabled(false);
	m_pEnergyLabel->SetPaintBorderEnabled(false);
	m_pEnergyLabel->SizeToContents();
	m_pEnergyLabel->SetContentAlignment(vgui::Label::a_west);
	m_pEnergyLabel->SetFgColor(GetFgColor());
}

void CResouceGathererResouceCounterHud::Reset()
{
}

void CResouceGathererResouceCounterHud::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetFgColor( Color(0,0,0,0) );	//GetSchemeColor("RoundStateFg", pScheme) );
	m_hFont = pScheme->GetFont( "Default", true );

	m_pBackground->SetBgColor( GetSchemeColor("BgColor", pScheme) );
	m_pBackground->SetPaintBackgroundType( 2 );

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

	int xOffset = (labelWide - m_pBiologicalLabel->GetWide())/2;
	m_pBiologicalLabel->SetPos( 0 + xOffset, 0 + m_iTextY );

	int totalY = 0;

	int size_x, size_y;
	m_pBiologicalLabel->GetSize(size_x, size_y);
	totalY += size_y;
	m_pMechanicalLabel->SetPos(0 + xOffset, m_pBiologicalLabel->GetYPos() + size_y + 5);

	m_pMechanicalLabel->GetSize(size_x, size_y);
	totalY += size_y;
	m_pEnergyLabel->SetPos(0 + xOffset, m_pMechanicalLabel->GetYPos() + size_y + 5);

	m_pBackground->SetBounds(0, 0, labelWide, m_pMechanicalLabel->GetYPos() + totalY + 10);
}

void CResouceGathererResouceCounterHud::OnThink()
{
    //SetVisible( false );

	// Biological Resource Count

    char szNum[6];
    Q_snprintf( szNum, sizeof(szNum), "%i", ResourceGathererRules()->GetBiologicalResourceCount() );
    wchar_t wNum[4];
    g_pVGuiLocalize->ConvertANSIToUnicode(szNum, wNum, sizeof(wNum));

    wchar_t bioLabel[1024];
    g_pVGuiLocalize->ConstructString( bioLabel, sizeof(bioLabel), g_pVGuiLocalize->Find("#ResourceGatherer_ResourceCounter_Biological"), 1, wNum );

    m_pBiologicalLabel->SetText( bioLabel );
    m_pBiologicalLabel->SetVisible( true );

    m_pBiologicalLabel->SizeToContents();

	// Mechanical Resource Count

	Q_snprintf(szNum, sizeof(szNum), "%i", ResourceGathererRules()->GetMechanicalResourceCount());
	g_pVGuiLocalize->ConvertANSIToUnicode(szNum, wNum, sizeof(wNum));

	wchar_t mechLabel[1024];
	g_pVGuiLocalize->ConstructString(mechLabel, sizeof(mechLabel), g_pVGuiLocalize->Find("#ResourceGatherer_ResourceCounter_Mechanical"), 1, wNum);

	m_pMechanicalLabel->SetText(mechLabel);
	m_pMechanicalLabel->SetVisible(true);

	m_pMechanicalLabel->SizeToContents();
	
	// Energy Resource Count

	Q_snprintf(szNum, sizeof(szNum), "%i", ResourceGathererRules()->GetEnergyResourceCount());
	g_pVGuiLocalize->ConvertANSIToUnicode(szNum, wNum, sizeof(wNum));

	wchar_t energyLabel[1024];
	g_pVGuiLocalize->ConstructString(energyLabel, sizeof(energyLabel), g_pVGuiLocalize->Find("#ResourceGatherer_ResourceCounter_Energy"), 1, wNum);

	m_pEnergyLabel->SetText(energyLabel);
	m_pEnergyLabel->SetVisible(true);

	m_pEnergyLabel->SizeToContents();

    InvalidateLayout();
}