// Enemy.cpp
#include "Enemy.h"
#include "Components/SphereComponent.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "TDDD23/InputPlayer/InputCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/DamageType.h"
#include "Animation/AnimInstance.h"     // NEW
#include "Animation/AnimMontage.h"      // NEW
#include "Animation/AnimSequence.h"     // NEW
#include "GameFramework/CharacterMovementComponent.h" // NEW

AEnemyNPC::AEnemyNPC()
{
	PrimaryActorTick.bCanEverTick = true;

	// Aggro sphere
	AggroSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AggroSphere"));
	AggroSphere->SetupAttachment(GetRootComponent());
	AggroSphere->InitSphereRadius(800.f);
	AggroSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AggroSphere->SetCollisionObjectType(ECC_WorldDynamic);
	AggroSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	AggroSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// Attack sphere
	AttackSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AttackSphere"));
	AttackSphere->SetupAttachment(GetRootComponent());
	AttackSphere->InitSphereRadius(150.f);
	AttackSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AttackSphere->SetCollisionObjectType(ECC_WorldDynamic);
	AttackSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	AttackSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// Bind overlaps
	AggroSphere->OnComponentBeginOverlap.AddDynamic(this, &AEnemyNPC::OnAggroBegin);
	AggroSphere->OnComponentEndOverlap.AddDynamic(this, &AEnemyNPC::OnAggroEnd);
	AttackSphere->OnComponentBeginOverlap.AddDynamic(this, &AEnemyNPC::OnAttackBegin);
	AttackSphere->OnComponentEndOverlap.AddDynamic(this, &AEnemyNPC::OnAttackEnd);

	// AI
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	Health = MaxHealth;

	//// ===== FIX: "sphere under him" / floating when placed =====
	//AggroSphere->bUseAttachParentBound = true;
	//AttackSphere->bUseAttachParentBound = true;
	//AggroSphere->SetCanEverAffectNavigation(false);
	//AttackSphere->SetCanEverAffectNavigation(false);
	//AggroSphere->SetHiddenInGame(true);
	//AttackSphere->SetHiddenInGame(true);

	AggroSphere->bUseAttachParentBound = true;
	AttackSphere->bUseAttachParentBound = true;
	AggroSphere->SetCanEverAffectNavigation(false);
	AttackSphere->SetCanEverAffectNavigation(false);
	AggroSphere->SetHiddenInGame(true);
	AttackSphere->SetHiddenInGame(true);

	// Make the AI face the controller’s desired rotation (we’ll drive it via SetFocus and FaceTarget)
	bUseControllerRotationYaw = true;
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->bUseControllerDesiredRotation = true;
		Move->bOrientRotationToMovement = false;    // let focus/manual facing drive yaw
		Move->RotationRate = FRotator(0.f, 720.f, 0.f); // how fast it turns
		Move->GravityScale = 1.f;                       // just to be explicit
	}
}


void AEnemyNPC::BeginPlay()
{
	Super::BeginPlay();

	// Nudge the mesh down so the feet touch the ground (prevents “floating” look). prev issue
	if (USkeletalMeshComponent* Skel = GetMesh())
	{
		FVector R = Skel->GetRelativeLocation();
		R.Z = MeshZOffset;
		Skel->SetRelativeLocation(R);

		// NEW: yaw offset so Paragon mesh visually faces the same way as the capsule
		FRotator RR = Skel->GetRelativeRotation();
		RR.Yaw += MeshYawOffset;
		Skel->SetRelativeRotation(RR);

		Skel->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	}

	// Start slow patrol until we see a player
	StartPatrol();
}

void AEnemyNPC::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Acquire target if none
	if (!bIsDead && !TargetPlayer)
	{
		if (AInputCharacter* P = Cast<AInputCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
		{
			const float Aggro = AggroSphere->GetScaledSphereRadius();
			if (FVector::DistSquared(P->GetActorLocation(), GetActorLocation()) <= Aggro * Aggro)
			{
				TargetPlayer = P;
				StartChasingTarget();
				BP_OnPlayerSpotted(P);
			}
		}
	}

	// Face the target whenever we have one (chasing or attacking)
	if (!bIsDead && TargetPlayer && bFaceTargetWhileChasing)
	{
		if (AAIController* AI = Cast<AAIController>(GetController()))
		{
			AI->SetFocus(TargetPlayer);
		}
		// Hard guarantee we keep turning toward the player even when path following
		FaceTarget(DeltaSeconds);
	}

	// Move while not in attack range
	if (!bIsDead && TargetPlayer && !bInAttackRange)
	{
		if (AAIController* AI = Cast<AAIController>(GetController()))
		{
			AI->MoveToActor(TargetPlayer, 0.f);
		}
	}

	// Evaluate attack range
	if (!bIsDead && TargetPlayer)
	{
		const float Range = AttackSphere->GetScaledSphereRadius();
		const float DistSq = FVector::DistSquared(TargetPlayer->GetActorLocation(), GetActorLocation());
		const bool  InRange = DistSq <= Range * Range;

		if (InRange && !bInAttackRange)
		{
			bInAttackRange = true;
			if (AAIController* AI = Cast<AAIController>(GetController()))
			{
				AI->StopMovement();
				AI->SetFocus(TargetPlayer);
			}
			StartAttack();
		}
		else if (!InRange && bInAttackRange)
		{
			bInAttackRange = false;
			StopAttack();
			StartChasingTarget();
			if (AAIController* AI = Cast<AAIController>(GetController()))
			{
				AI->ClearFocus(EAIFocusPriority::Gameplay);
			}
		}
	}

	// Patrol when idle
	if (!bIsDead && !TargetPlayer)
	{
		AdvancePatrolIfClose();
	}

	// --- Drive locomotion from C++ when not attacking ---
	if (!bIsDead)
	{
		const float Speed = GetVelocity().Size2D();

		// If attacking, do nothing here; the attack montage is in charge.
		if (!IsAnyAttackMontagePlaying())
		{
			if (Speed >= 10.f)
			{
				// Pick Jog vs Run sequence
				UAnimSequence* MoveSeq = (Speed < RunSpeedThreshold)
					? (Jog_Fwd_Sequence ? Jog_Fwd_Sequence : Run_Fwd_Sequence)
					: (Run_Fwd_Sequence ? Run_Fwd_Sequence : Jog_Fwd_Sequence);

				// Enter Single-Node locomotion if not already (or if the sequence changed)
				if (!bUsingSingleNodeLocomotion || CurrentLocomotionSeq != MoveSeq)
				{
					EnterSingleNodeLocomotion(MoveSeq);
					CurrentLocomotionSeq = MoveSeq; // reuse for change detection
				}
			}
			else
			{
				// Not moving: go back to ABP so Idle shows
				if (bUsingSingleNodeLocomotion)
				{
					ExitSingleNodeLocomotion();
					CurrentLocomotionSeq = nullptr;
				}
				// (We don't call EnsureLocomotion(Idle_Sequence) here; ABP handles Idle.)
			}
		}
	}
}

void AEnemyNPC::OnAggroBegin(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*,
	int32, bool, const FHitResult&)
{
	if (bIsDead || TargetPlayer) return;

	if (AInputCharacter* Player = Cast<AInputCharacter>(Other))
	{
		TargetPlayer = Player;
		StartChasingTarget();
		BP_OnPlayerSpotted(Player);
	}
}

void AEnemyNPC::OnAggroEnd(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*,
	int32)
{
	if (Other == TargetPlayer)
	{
		bInAttackRange = false;
		StopAttack();
		StopChasingTarget();
		TargetPlayer = nullptr;
		BP_OnPlayerLost();

		// Resume patrol when target is lost
		StartPatrol();
	}
}

void AEnemyNPC::OnAttackBegin(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*,
	int32, bool, const FHitResult&)
{
	if (bIsDead) return;
	if (Other == TargetPlayer)
	{
		bInAttackRange = true;
		if (AAIController* AI = Cast<AAIController>(GetController()))
		{
			AI->StopMovement();
			AI->SetFocus(TargetPlayer);
		}
		StartAttack();
	}
}

void AEnemyNPC::OnAttackEnd(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*,
	int32)
{
	if (Other == TargetPlayer)
	{
		bInAttackRange = false;
		StopAttack();
		StartChasingTarget();
		if (AAIController* AI = Cast<AAIController>(GetController()))
		{
			AI->ClearFocus(EAIFocusPriority::Gameplay);
		}
	}
}

void AEnemyNPC::StartChasingTarget()
{
	if (!TargetPlayer || bIsDead) return;

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
		Move->MaxWalkSpeed = ChaseSpeed;

	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		if (bFaceTargetWhileChasing) { AI->SetFocus(TargetPlayer); }
		AI->MoveToActor(TargetPlayer, 0.f);
	}
	else
	{
		UAIBlueprintHelperLibrary::SimpleMoveToActor(GetController(), TargetPlayer);
	}
}

void AEnemyNPC::StopChasingTarget()
{
	if (bUsingSingleNodeLocomotion)
	{
		ExitSingleNodeLocomotion();
	}
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		AI->StopMovement();
		AI->ClearFocus(EAIFocusPriority::Gameplay);
	}
}

void AEnemyNPC::StartAttack()
{
	if (bUsingSingleNodeLocomotion)
	{
		ExitSingleNodeLocomotion();
	}
	StopLocomotion();
	if (bIsDead || !TargetPlayer) return;
	if (GetWorldTimerManager().IsTimerActive(AttackTimerHandle)) return;

	// Snap to target so the swing faces the player
	if (bSnapYawOnAttack && TargetPlayer)
	{
		const FVector To = (TargetPlayer->GetActorLocation() - GetActorLocation());
		FRotator Snap = To.Rotation();
		Snap.Pitch = Snap.Roll = 0.f;
		if (AAIController* AI = Cast<AAIController>(GetController()))
		{
			AI->SetControlRotation(Snap);
		}
		SetActorRotation(Snap);
	}

	// === Play one of the Paragon attack montages right away ===
	if (UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		// Randomly choose right/left swing if both are set
		UAnimMontage* MontageToPlay = PrimaryAttack_RA_Montage;
		if (PrimaryAttack_LA_Montage && FMath::RandBool())
		{
			MontageToPlay = PrimaryAttack_LA_Montage;
		}

		if (MontageToPlay)
		{
			Anim->Montage_Play(MontageToPlay, 1.f);
		}
	}

	// Continue using your timer-driven damage (simple & reliable)
	GetWorldTimerManager().SetTimer(
		AttackTimerHandle, this, &AEnemyNPC::PerformAttack,
		AttackInterval, true, FirstHitDelay);
}

void AEnemyNPC::StopAttack()
{
	GetWorldTimerManager().ClearTimer(AttackTimerHandle);
}

void AEnemyNPC::PerformAttack()
{
	if (bIsDead || !TargetPlayer) return;

	// Ensure montage is playing each swing; if not, trigger it again
	if (UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		if (PrimaryAttack_RA_Montage || PrimaryAttack_LA_Montage)
		{
			if (!(PrimaryAttack_RA_Montage && Anim->Montage_IsPlaying(PrimaryAttack_RA_Montage)) &&
				!(PrimaryAttack_LA_Montage && Anim->Montage_IsPlaying(PrimaryAttack_LA_Montage)))
			{
				// re-trigger one of the swings
				Anim->Montage_Play(PrimaryAttack_RA_Montage ? PrimaryAttack_RA_Montage : PrimaryAttack_LA_Montage, 1.f);
			}
		}
	}

	// Range check & damage
	const float DistSq = FVector::DistSquared(TargetPlayer->GetActorLocation(), GetActorLocation());
	const float Range = AttackSphere->GetScaledSphereRadius();
	if (DistSq <= Range * Range)
	{
		BP_OnAttack(); // keep for VFX/SFX
		UGameplayStatics::ApplyDamage(TargetPlayer, Damage, GetController(), this, UDamageType::StaticClass());
	}
	else
	{
		bInAttackRange = false;
		StopAttack();
		StartChasingTarget();
	}
}

float AEnemyNPC::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead) return 0.f;

	const float Actual = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (Actual <= 0.f) return 0.f;

	Health = FMath::Clamp(Health - Actual, 0.f, MaxHealth);
	BP_OnDamaged(Actual);

	// === Play a direction-aware hit react (short, non-loop) ===
	const FVector From = DamageCauser ? DamageCauser->GetActorLocation()
		: (EventInstigator && EventInstigator->GetPawn() ? EventInstigator->GetPawn()->GetActorLocation() : GetActorLocation() - GetActorForwardVector() * 100.f);
	PlayHitReactFrom(From);

	if (Health <= 0.f)
	{
		bIsDead = true;
		StopAttack();
		StopChasingTarget();

		// Disable capsule; keep mesh; play death
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		// Play "Death" (AnimSequence). Prefer dynamic montage so it blends cleanly.
		if (UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			if (Death_Sequence)
			{
				UAnimMontage* Dyn = Anim->PlaySlotAnimationAsDynamicMontage(
					Death_Sequence, MontageSlotName, 0.2f, 0.2f, 1.f, 1);
				if (!Dyn)
				{
					// Fallback if no Slot exists in AnimBP
					GetMesh()->PlayAnimation(Death_Sequence, false);
				}
			}
		}

		BP_OnDeath();
		SetLifeSpan(5.f);
	}
	return Actual;
}

// ===== NEW =====
void AEnemyNPC::PlayHitReactFrom(const FVector& FromWorldPos)
{
	if (bIsDead) return;

	const FVector DirToMe = (GetActorLocation() - FromWorldPos).GetSafeNormal();
	const float FwdDot = FVector::DotProduct(GetActorForwardVector(), DirToMe);
	const float RightDot = FVector::DotProduct(GetActorRightVector(), DirToMe);

	UAnimSequence* Chosen = nullptr;
	if (FMath::Abs(FwdDot) >= FMath::Abs(RightDot))
	{
		Chosen = (FwdDot >= 0.f) ? HitReact_Front : HitReact_Back;
	}
	else
	{
		Chosen = (RightDot >= 0.f) ? HitReact_Right : HitReact_Left;
	}

	if (!Chosen) return;

	if (UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		// Try layered play via dynamic montage (needs a Slot in the AnimBP named MontageSlotName)
		UAnimMontage* Dyn = Anim->PlaySlotAnimationAsDynamicMontage(Chosen, MontageSlotName, 0.1f, 0.12f, 1.f, 1);
		if (!Dyn)
		{
			// Fallback: play the sequence directly (will temporarily take over the pose)
			GetMesh()->PlayAnimation(Chosen, false);
		}
	}
}

// ===== NEW: Patrol, very small loop =====
void AEnemyNPC::StartPatrol()
{
	if (bIsDead || PatrolPoints.Num() == 0) return;

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->MaxWalkSpeed = PatrolSpeed; // slower while patrolling
	}

	CurrentPatrolIndex = FMath::Clamp(CurrentPatrolIndex, 0, FMath::Max(0, PatrolPoints.Num() - 1));
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		if (AActor* Goal = PatrolPoints[CurrentPatrolIndex])
		{
			AI->MoveToActor(Goal, PatrolAcceptanceRadius);
		}
	}
}

void AEnemyNPC::AdvancePatrolIfClose()
{
	if (PatrolPoints.Num() == 0) return;

	if (AActor* Goal = PatrolPoints[CurrentPatrolIndex])
	{
		const float DistSq = FVector::DistSquared(GetActorLocation(), Goal->GetActorLocation());
		if (DistSq <= FMath::Square(PatrolAcceptanceRadius + 25.f))
		{
			CurrentPatrolIndex = (CurrentPatrolIndex + 1) % PatrolPoints.Num();
			StartPatrol();
		}
	}
}

bool AEnemyNPC::IsAnyAttackMontagePlaying() const
{
	if (const UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		if (PrimaryAttack_RA_Montage && Anim->Montage_IsPlaying(PrimaryAttack_RA_Montage)) return true;
		if (PrimaryAttack_LA_Montage && Anim->Montage_IsPlaying(PrimaryAttack_LA_Montage)) return true;
	}
	return false;
}

FName AEnemyNPC::ResolveLocomotionSlot() const
{
	// If user set a specific slot name and it's not the placeholder "DefaultSlot", use it.
	if (LocomotionSlotName != NAME_None && LocomotionSlotName != FName(TEXT("DefaultSlot")))
	{
		return LocomotionSlotName;
	}

	// Otherwise, reuse the first slot defined by our attack montages (guaranteed to exist in the ABP)
	if (PrimaryAttack_RA_Montage && PrimaryAttack_RA_Montage->SlotAnimTracks.Num() > 0)
	{
		return PrimaryAttack_RA_Montage->SlotAnimTracks[0].SlotName;
	}
	if (PrimaryAttack_LA_Montage && PrimaryAttack_LA_Montage->SlotAnimTracks.Num() > 0)
	{
		return PrimaryAttack_LA_Montage->SlotAnimTracks[0].SlotName;
	}

	// Last resort ( Paragon name)
	return FName(TEXT("FullBody"));
}


void AEnemyNPC::EnsureLocomotion(UAnimSequence* Seq)
{
	if (!Seq || bIsDead) return;

	UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!Anim) return;

	// Don't start locomotion if an attack montage is active
	if (IsAnyAttackMontagePlaying()) return;

	// Already playing this sequence? keep it
	if (CurrentLocomotionSeq == Seq && CurrentLocomotionMontage && Anim->Montage_IsPlaying(CurrentLocomotionMontage))
		return;

	// Stop previous locomotion montage (soft blend)
	if (CurrentLocomotionMontage)
	{
		Anim->Montage_Stop(0.2f, CurrentLocomotionMontage);
		CurrentLocomotionMontage = nullptr;
		CurrentLocomotionSeq = nullptr;
	}

	// Use a valid slot (taken from attack montage if LocomotionSlotName is None)
	const FName Slot = ResolveLocomotionSlot();

	CurrentLocomotionMontage = Anim->PlaySlotAnimationAsDynamicMontage(
		Seq, Slot, /*blendIn*/0.2f, /*blendOut*/0.2f, /*rate*/1.f, /*numLoops*/0 /*loop*/);

	if (!CurrentLocomotionMontage)
	{
		// If this logs, your AnimBP is missing the slot. Add a Slot node with the same name.
		UE_LOG(LogTemp, Warning, TEXT("EnsureLocomotion: AnimBP missing slot '%s'"), *Slot.ToString());
		return;
	}

	CurrentLocomotionSeq = Seq;
}


void AEnemyNPC::StopLocomotion()
{
	if (UAnimInstance* Anim = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		if (CurrentLocomotionMontage)
		{
			Anim->Montage_Stop(0.15f, CurrentLocomotionMontage);
		}
	}
	CurrentLocomotionMontage = nullptr;
	CurrentLocomotionSeq = nullptr;
}

void AEnemyNPC::FaceTarget(float DeltaSeconds)
{
	if (!TargetPlayer || bIsDead) return;

	const FVector ToTarget = TargetPlayer->GetActorLocation() - GetActorLocation();
	FRotator Desired = ToTarget.Rotation();
	Desired.Pitch = 0.f;
	Desired.Roll = 0.f;

	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		const FRotator NewCR = FMath::RInterpTo(AI->GetControlRotation(), Desired, DeltaSeconds, FaceInterpSpeed);
		AI->SetControlRotation(NewCR);
	}
	else
	{
		const FRotator NewYaw = FMath::RInterpTo(GetActorRotation(), Desired, DeltaSeconds, FaceInterpSpeed);
		SetActorRotation(NewYaw);
	}
}
void AEnemyNPC::EnterSingleNodeLocomotion(UAnimSequence* SeqToLoop)
{
	if (bIsDead || !SeqToLoop) return;
	if (!GetMesh()) return;

	// Switch to Single Node mode and loop the sequence
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	GetMesh()->PlayAnimation(SeqToLoop, /*bLoop=*/true);
	bUsingSingleNodeLocomotion = true;
}

void AEnemyNPC::ExitSingleNodeLocomotion()
{
	if (!GetMesh()) return;

	// Return to ABP mode so montages and normal ABP logic work (idle/attacks)
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	// Clear any single-node playback
	GetMesh()->Stop();
	bUsingSingleNodeLocomotion = false;
}

