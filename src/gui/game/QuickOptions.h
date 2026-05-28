#include "QuickOption.h"

class GameController;

class SandEffectOption: public QuickOption
{
public:
	SandEffectOption(GameModel * m);
	bool GetToggle() override;
	void perform() override;
};

class DrawGravOption: public QuickOption
{
public:
	DrawGravOption(GameModel * m);
	bool GetToggle() override;
	void perform() override;
};

class DrawMagneticOption: public QuickOption
{
public:
	DrawMagneticOption(GameModel * m);
	bool GetToggle() override;
	void perform() override;
};

class DecorationsOption: public QuickOption
{
public:
	DecorationsOption(GameModel * m);
	bool GetToggle() override;
	void perform() override;
};

class NGravityOption: public QuickOption
{
public:
	NGravityOption(GameModel * m);
	bool GetToggle() override;
	void perform() override;
};

class MagnetismEnableOption: public QuickOption
{
public:
	MagnetismEnableOption(GameModel * m);
	bool GetToggle() override;
	void perform() override;
};

class DrawElectricOption: public QuickOption
{
public:
	DrawElectricOption(GameModel * m);
	bool GetToggle() override;
	void perform() override;
};

class ElectricityEnableOption: public QuickOption
{
public:
	ElectricityEnableOption(GameModel * m);
	bool GetToggle() override;
	void perform() override;
};

class AHeatOption: public QuickOption
{
public:
	AHeatOption(GameModel * m);
	bool GetToggle() override;
	void perform() override;
};

class ConsoleShowOption: public QuickOption
{
	GameController * c;
public:
	ConsoleShowOption(GameModel * m, GameController * c_);
	bool GetToggle() override;
	void perform() override;
};
