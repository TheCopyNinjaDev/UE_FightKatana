// Copyright Epic Games, Inc. All Rights Reserved.

#include "UE_FightKatanaGameMode.h"
#include "UE_FightKatanaCharacter.h"
#include "UObject/ConstructorHelpers.h"

AUE_FightKatanaGameMode::AUE_FightKatanaGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
