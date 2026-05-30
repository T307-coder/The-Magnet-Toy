#include "QuickOptions.h"

#include "GameModel.h"
#include "GameController.h"

#include "simulation/Simulation.h"

SandEffectOption::SandEffectOption(GameModel * m):
QuickOption("P", "Sand effect", m, Toggle)
{

}
bool SandEffectOption::GetToggle()
{
	return m->GetSimulation()->pretty_powder;
}
void SandEffectOption::perform()
{
	m->GetSimulation()->pretty_powder = !m->GetSimulation()->pretty_powder;
}



DrawGravOption::DrawGravOption(GameModel * m):
QuickOption("G", "Draw gravity field \bg(ctrl+g)", m, Toggle)
{

}
bool DrawGravOption::GetToggle()
{
	return m->GetGravityGrid();
}
void DrawGravOption::perform()
{
	m->ShowGravityGrid(!m->GetGravityGrid());
}



DrawMagneticOption::DrawMagneticOption(GameModel * m):
QuickOption("M", "Draw magnetic field", m, Toggle)
{

}
bool DrawMagneticOption::GetToggle()
{
	return m->GetMagneticField();
}
void DrawMagneticOption::perform()
{
	m->ShowMagneticField(!m->GetMagneticField());
}



DecorationsOption::DecorationsOption(GameModel * m):
QuickOption("D", "Draw decorations \bg(ctrl+b)", m, Toggle)
{

}
bool DecorationsOption::GetToggle()
{
	return m->GetDecoration();
}
void DecorationsOption::perform()
{
	m->SetDecoration(!m->GetDecoration());
}



NGravityOption::NGravityOption(GameModel * m):
QuickOption("N", "Newtonian Gravity \bg(n)", m, Toggle)
{

}
bool NGravityOption::GetToggle()
{
	return m->GetNewtonianGrvity();
}
void NGravityOption::perform()
{
	m->SetNewtonianGravity(!m->GetNewtonianGrvity());
}



MagnetismEnableOption::MagnetismEnableOption(GameModel * m):
QuickOption("B", "Magnetism simulation \uE001", m, Toggle)
{

}
bool MagnetismEnableOption::GetToggle()
{
	return m->GetMagnetismEnabled();
}
void MagnetismEnableOption::perform()
{
	m->SetMagnetismEnabled(!m->GetMagnetismEnabled());
}



DrawElectricOption::DrawElectricOption(GameModel * m):
QuickOption("E", "Draw electric field", m, Toggle)
{

}
bool DrawElectricOption::GetToggle()
{
	return m->GetElectricField();
}
void DrawElectricOption::perform()
{
	m->ShowElectricField(!m->GetElectricField());
}



ElectricityEnableOption::ElectricityEnableOption(GameModel * m):
QuickOption("Y", "Electricity simulation", m, Toggle)
{

}
bool ElectricityEnableOption::GetToggle()
{
	return m->GetElectricityEnabled();
}
void ElectricityEnableOption::perform()
{
	m->SetElectricityEnabled(!m->GetElectricityEnabled());
}



AHeatOption::AHeatOption(GameModel * m):
QuickOption("A", "Ambient heat \bg(u)", m, Toggle)
{

}
bool AHeatOption::GetToggle()
{
	return m->GetAHeatEnable();
}
void AHeatOption::perform()
{
	m->SetAHeatEnable(!m->GetAHeatEnable());
}



ConsoleShowOption::ConsoleShowOption(GameModel * m, GameController * c_):
QuickOption("C", "Show Console \bg(~)", m, Toggle)
{
	c = c_;
}
bool ConsoleShowOption::GetToggle()
{
	return 0;
}
void ConsoleShowOption::perform()
{
	c->ShowConsole();
}

InductionEnableOption::InductionEnableOption(GameModel * m):
QuickOption("I", "Magnetic induction \bg(dB/dt sparking)", m, Toggle)
{

}
bool InductionEnableOption::GetToggle()
{
	return m->GetInductionEnabled();
}
void InductionEnableOption::perform()
{
	m->SetInductionEnabled(!m->GetInductionEnabled());
}

CurrentBFieldOption::CurrentBFieldOption(GameModel * m):
QuickOption("J", "Current magnetic field \bg(Biot-Savart)", m, Toggle)
{

}
bool CurrentBFieldOption::GetToggle()
{
	return m->GetCurrentBFieldEnabled();
}
void CurrentBFieldOption::perform()
{
	m->SetCurrentBFieldEnabled(!m->GetCurrentBFieldEnabled());
}

RealisticPstnOption::RealisticPstnOption(GameModel * m):
QuickOption("R", "Realistic PSTN \bg(gives velocity)", m, Toggle)
{

}
bool RealisticPstnOption::GetToggle()
{
	return m->GetRealisticPstnEnabled();
}
void RealisticPstnOption::perform()
{
	m->SetRealisticPstnEnabled(!m->GetRealisticPstnEnabled());
}

SprkCurrentOption::SprkCurrentOption(GameModel * m):
QuickOption("K", "SPRK current magnetic field", m, Toggle)
{

}
bool SprkCurrentOption::GetToggle()
{
	return m->GetSprkCurrentEnabled();
}
void SprkCurrentOption::perform()
{
	m->SetSprkCurrentEnabled(!m->GetSprkCurrentEnabled());
}


