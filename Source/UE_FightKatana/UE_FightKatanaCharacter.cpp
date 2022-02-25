// Copyright Epic Games, Inc. All Rights Reserved.

#include "UE_FightKatanaCharacter.h"

#include "BlueprintEditor.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

//////////////////////////////////////////////////////////////////////////
// AUE_FightKatanaCharacter

AUE_FightKatanaCharacter::AUE_FightKatanaCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rate for input
	TurnRateGamepad = 50.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

//////////////////////////////////////////////////////////////////////////
// Input

void AUE_FightKatanaCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction("LockTarget", IE_Pressed, this, &AUE_FightKatanaCharacter::LockTarget);

	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &AUE_FightKatanaCharacter::MoveForward);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &AUE_FightKatanaCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnRate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn Right / Left Mouse", this, &AUE_FightKatanaCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("Turn Right / Left Gamepad", this, &AUE_FightKatanaCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("Look Up / Down Mouse", this, &AUE_FightKatanaCharacter::LookUpAtRate);
	PlayerInputComponent->BindAxis("Look Up / Down Gamepad", this, &AUE_FightKatanaCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AUE_FightKatanaCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AUE_FightKatanaCharacter::TouchStopped);
}
void AUE_FightKatanaCharacter::LockTarget()
{
	if(IsLockedOnEnemy)
	{
		LockedOnTarget = nullptr;
		IsLockedOnEnemy = false;
		bUseControllerRotationYaw = false;
	}
	else
	{
		FVector StartLocation = GetActorLocation();
		FVector EndLocation = FollowCamera->GetForwardVector() * 1000 + GetActorLocation();
		TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypesArray;
		ObjectTypesArray.Reserve(1);
		ObjectTypesArray.Emplace(ObjectTypeQuery4);

		const TArray<AActor*> ActorsToIgnore;

		if(FHitResult OutHit; UKismetSystemLibrary::SphereTraceSingleForObjects(
			GetWorld(),
			StartLocation,
			EndLocation,
			300,
			ObjectTypesArray,
			false,
			ActorsToIgnore,
			EDrawDebugTrace::None,
			OutHit,
			true))
		{
			if(IsValid(OutHit.GetActor()) && OutHit.GetActor()->Tags.Contains("EnemyTag"))
			{
				LockedOnTarget = OutHit.GetActor();
				IsLockedOnEnemy = true;
				bUseControllerRotationYaw = true;
			}
		}
	}

	
}

// Called every frame
void AUE_FightKatanaCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if(IsLockedOnEnemy)
	{
		// Find current rotator
		const FRotator Current = GetController()->K2_GetActorRotation();

		// Find target rotator
		FVector ActorLocation = LockedOnTarget->GetActorLocation();
		ActorLocation.Set(ActorLocation.X, ActorLocation.Y, ActorLocation.Z - 200);
		const FRotator Target = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), ActorLocation);

		const FRotator Interpretation = FMath::RInterpTo(Current, Target, DeltaTime, 10);
		FRotator ResultRotator;
		ResultRotator.Roll = Current.Roll;
		ResultRotator.Pitch = Interpretation.Pitch;
		ResultRotator.Yaw = Interpretation.Yaw;

		
		GetController()->SetControlRotation(ResultRotator);
	}
}

void AUE_FightKatanaCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	Jump();
}

void AUE_FightKatanaCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	StopJumping();
}

void AUE_FightKatanaCharacter::TurnAtRate(float Rate)
{
	if(!IsLockedOnEnemy)
	{
		// calculate delta for this frame from the rate information
		AddControllerYawInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
	}
}

void AUE_FightKatanaCharacter::LookUpAtRate(float Rate)
{
	if(!IsLockedOnEnemy)
	{
		// calculate delta for this frame from the rate information
		AddControllerPitchInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
	}
}

void AUE_FightKatanaCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AUE_FightKatanaCharacter::MoveRight(float Value)
{
	if ( (Controller != nullptr) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}
