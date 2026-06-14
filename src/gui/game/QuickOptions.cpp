#include "QuickOptions.h"

#include "GameModel.h"
#include "GameController.h"
#include "common/Localization.h"
#include "simulation/Simulation.h"

SandEffectOption::SandEffectOption(GameModel * m):
QuickOption("P", Localization::Ref().Tr("quick.sand_effect"), m, Toggle)
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
QuickOption("G", Localization::Ref().Tr("quick.draw_gravity"), m, Toggle)
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
QuickOption("M", Localization::Ref().Tr("quick.draw_magnetic"), m, Toggle)
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
QuickOption("D", Localization::Ref().Tr("quick.draw_decorations"), m, Toggle)
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
QuickOption("N", Localization::Ref().Tr("quick.newtonian_gravity"), m, Toggle)
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
QuickOption("B", Localization::Ref().Tr("quick.magnetism_enable") + " \uE001", m, Toggle)
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
QuickOption("E", Localization::Ref().Tr("quick.draw_electric"), m, Toggle)
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
QuickOption("Y", Localization::Ref().Tr("quick.electricity_enable"), m, Toggle)
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
QuickOption("A", Localization::Ref().Tr("quick.ambient_heat"), m, Toggle)
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
QuickOption("C", Localization::Ref().Tr("quick.show_console"), m, Toggle)
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
QuickOption("I", Localization::Ref().Tr("quick.magnetic_induction") + " \bg(dB/dt sparking, not recommended)", m, Toggle)
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
QuickOption("J", Localization::Ref().Tr("quick.current_bfield") + " \bg(Biot-Savart)", m, Toggle)
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
QuickOption("R", Localization::Ref().Tr("quick.realistic_pstn") + " \bg(gives velocity)", m, Toggle)
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
QuickOption("K", Localization::Ref().Tr("quick.sprk_current"), m, Toggle)
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

CoilMagnetizeOption::CoilMagnetizeOption(GameModel * m):
QuickOption("X", Localization::Ref().Tr("quick.coil_magnetize") + " \bg(SPRK charges magnets)", m, Toggle)
{

}
bool CoilMagnetizeOption::GetToggle()
{
	return m->GetCoilMagnetizeEnabled();
}
void CoilMagnetizeOption::perform()
{
	m->SetCoilMagnetizeEnabled(!m->GetCoilMagnetizeEnabled());
}

TriboElectricOption::TriboElectricOption(GameModel * m):
QuickOption("T", Localization::Ref().Tr("quick.triboelectricity") + " \bg(insulator contact charging)", m, Toggle)
{

}
bool TriboElectricOption::GetToggle()
{
	return m->GetTriboElectricEnabled();
}
void TriboElectricOption::perform()
{
	m->SetTriboElectricEnabled(!m->GetTriboElectricEnabled());
}

FreeChargeFieldsOption::FreeChargeFieldsOption(GameModel * m):
QuickOption("Q", Localization::Ref().Tr("quick.free_charge") + " \bg(powders/liquids/gases)", m, Toggle)
{

}
bool FreeChargeFieldsOption::GetToggle()
{
	return m->GetFreeChargeFieldsEnabled();
}
void FreeChargeFieldsOption::perform()
{
	m->SetFreeChargeFieldsEnabled(!m->GetFreeChargeFieldsEnabled());
}

NewInductionOption::NewInductionOption(GameModel * m):
QuickOption("O", Localization::Ref().Tr("quick.new_induction") + " \bg(dB/dt charge transfer)", m, Toggle)
{

}
bool NewInductionOption::GetToggle()
{
	return m->GetNewInductionEnabled();
}
void NewInductionOption::perform()
{
	m->SetNewInductionEnabled(!m->GetNewInductionEnabled());
}

InductionSprkOption::InductionSprkOption(GameModel * m):
QuickOption("Z", Localization::Ref().Tr("quick.induction_sprk") + " \bg(auto-spark from high charge)", m, Toggle)
{

}
bool InductionSprkOption::GetToggle()
{
	return m->GetInductionSprkEnabled();
}
void InductionSprkOption::perform()
{
	m->SetInductionSprkEnabled(!m->GetInductionSprkEnabled());
}

PotentialCurrentOption::PotentialCurrentOption(GameModel * m):
QuickOption("S", Localization::Ref().Tr("quick.potential_current") + " \bg(SPRK to higher V)", m, Toggle)
{

}
bool PotentialCurrentOption::GetToggle()
{
	return m->GetPotentialCurrentEnabled();
}
void PotentialCurrentOption::perform()
{
	m->SetPotentialCurrentEnabled(!m->GetPotentialCurrentEnabled());
}

PolarizationOption::PolarizationOption(GameModel * m):
QuickOption("U", Localization::Ref().Tr("quick.polarization") + " \bg(E-field)", m, Toggle)
{

}
bool PolarizationOption::GetToggle()
{
	return m->GetPolarizationEnabled();
}
void PolarizationOption::perform()
{
	m->SetPolarizationEnabled(!m->GetPolarizationEnabled());
}

ParticleGravityOption::ParticleGravityOption(GameModel * m):
QuickOption("L", Localization::Ref().Tr("quick.particle_gravity") + " \bg(all particles source gravity)", m, Toggle)
{

}
bool ParticleGravityOption::GetToggle()
{
	return m->GetParticleGravityEnabled();
}
void ParticleGravityOption::perform()
{
	m->SetParticleGravityEnabled(!m->GetParticleGravityEnabled());
}

NonferroFieldsOption::NonferroFieldsOption(GameModel * m):
QuickOption("F", Localization::Ref().Tr("quick.nonferro_force") + " \bg(gradient pull/push)", m, Toggle)
{

}
bool NonferroFieldsOption::GetToggle()
{
	return m->GetNonferroFieldsEnabled();
}
void NonferroFieldsOption::perform()
{
	m->SetNonferroFieldsEnabled(!m->GetNonferroFieldsEnabled());
}

EddyCurrentOption::EddyCurrentOption(GameModel * m):
QuickOption("V", Localization::Ref().Tr("quick.eddy_current") + " \bg(induction heating)", m, Toggle)
{

}
bool EddyCurrentOption::GetToggle()
{
	return m->GetEddyCurrentEnabled();
}
void EddyCurrentOption::perform()
{
	m->SetEddyCurrentEnabled(!m->GetEddyCurrentEnabled());
}

CurieQuenchOption::CurieQuenchOption(GameModel * m):
QuickOption("W", Localization::Ref().Tr("quick.curie_quench") + " \bg(freeze domains at 773K)", m, Toggle)
{

}
bool CurieQuenchOption::GetToggle()
{
	return m->GetCurieQuenchEnabled();
}
void CurieQuenchOption::perform()
{
	m->SetCurieQuenchEnabled(!m->GetCurieQuenchEnabled());
}
