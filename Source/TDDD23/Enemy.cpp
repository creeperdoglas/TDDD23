
#include "Enemy.h"
#include "Components/SphereComponent.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "TDDD23/InputPlayer/InputCharacter.h" // needed for Cast<AInputCharacter>(...)
#include "Components/CapsuleComponent.h"       // GetCapsuleComponent()->SetCollisionEnabled(...)
#include "GameFramework/DamageType.h"          // UDamageType::StaticClass()
//On death, you already disable the capsule. If you ragdoll or want full cleanup, you can also disable the mesh’s collision:
//GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
//ta en titt på det sen


AEnemyNPC::AEnemyNPC()
{
	PrimaryActorTick.bCanEverTick = true;

	//skapa spheres
	AggroSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AggroSphere"));
	AggroSphere->SetupAttachment(GetRootComponent());
	AggroSphere->InitSphereRadius(800.f);
	AggroSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AggroSphere->SetCollisionObjectType(ECC_WorldDynamic);
	AggroSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	AggroSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	AttackSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AttackSphere"));
	AttackSphere->SetupAttachment(GetRootComponent());
	AttackSphere->InitSphereRadius(150.f);
	AttackSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AttackSphere->SetCollisionObjectType(ECC_WorldDynamic);
	AttackSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	AttackSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// Bind overlap events
	AggroSphere->OnComponentBeginOverlap.AddDynamic(this, &AEnemyNPC::OnAggroBegin);
	AggroSphere->OnComponentEndOverlap.AddDynamic(this, &AEnemyNPC::OnAggroEnd);
	AttackSphere->OnComponentBeginOverlap.AddDynamic(this, &AEnemyNPC::OnAttackBegin);
	AttackSphere->OnComponentEndOverlap.AddDynamic(this, &AEnemyNPC::OnAttackEnd);

	
	//se till att "pawn" runs ai när placerad eller spawned
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	Health = MaxHealth;
}

void AEnemyNPC::BeginPlay()
{
	Super::BeginPlay();
}

void AEnemyNPC::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

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
	// Keep moving only while not in attack range
	if (!bIsDead && TargetPlayer && !bInAttackRange)
	{
		if (AAIController* AI = Cast<AAIController>(GetController()))
		{
			AI->MoveToActor(TargetPlayer, 0.f); // good change
		}
	}

	// Always evaluate attack range whenever we have a target
	if (!bIsDead && TargetPlayer)
	{
		const float Range = AttackSphere->GetScaledSphereRadius();
		const float DistSq = FVector::DistSquared(TargetPlayer->GetActorLocation(), GetActorLocation());
		const bool InRange = DistSq <= Range * Range;

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
}

void AEnemyNPC::OnAggroBegin(UPrimitiveComponent* /*Overlapped*/, AActor* Other, UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/, bool /*bFromSweep*/, const FHitResult& /*SweepResult*/)
{
	if (bIsDead || TargetPlayer) return;

	if (AInputCharacter* Player = Cast<AInputCharacter>(Other))
	{
		TargetPlayer = Player;
		StartChasingTarget();
		BP_OnPlayerSpotted(Player);
	}

	/*if (!bIsDead && !TargetPlayer)
	{
		AInputCharacter* P = Cast<AInputCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
		if (P)
		{
			const float Aggro = AggroSphere->GetScaledSphereRadius();
			if (FVector::DistSquared(P->GetActorLocation(), GetActorLocation()) <= Aggro * Aggro)
			{
				TargetPlayer = P;
				StartChasingTarget();
				BP_OnPlayerSpotted(P);
			}
		}*/
	//}
}

void AEnemyNPC::OnAggroEnd(UPrimitiveComponent* /*Overlapped*/, AActor* Other, UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/)
{
	if (Other == TargetPlayer)
	{
		bInAttackRange = false;
		StopAttack();
		StopChasingTarget();
		TargetPlayer = nullptr;
		BP_OnPlayerLost();
	}
}

void AEnemyNPC::OnAttackBegin(UPrimitiveComponent* /*Overlapped*/, AActor* Other, UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/, bool /*bFromSweep*/, const FHitResult& /*SweepResult*/)
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

void AEnemyNPC::OnAttackEnd(UPrimitiveComponent* /*Overlapped*/, AActor* Other, UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/)
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

	// båda är ok; använder AIController::MoveToActor här
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		//AI->MoveToActor(TargetPlayer, AttackSphere->GetScaledSphereRadius());
		AI->MoveToActor(TargetPlayer, 0.f);
	}
	else
	{
		UAIBlueprintHelperLibrary::SimpleMoveToActor(GetController(), TargetPlayer);
	}
}

void AEnemyNPC::StopChasingTarget()
{
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		AI->StopMovement();
		AI->ClearFocus(EAIFocusPriority::Gameplay);
	}
}

void AEnemyNPC::StartAttack()
{
	if (bIsDead || !TargetPlayer) return;
	if (GetWorldTimerManager().IsTimerActive(AttackTimerHandle)) return;

	//först slag lite delayed, sen är de på attack interval
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

	// Double-check still in range
	const float DistSq = FVector::DistSquared(TargetPlayer->GetActorLocation(), GetActorLocation());
	const float Range = AttackSphere->GetScaledSphereRadius();
	if (DistSq <= Range * Range)
	{
		BP_OnAttack(); // låt BP spela animation eller liknande

		// körs in i AInputCharacter::TakeDamage på spelare
		UGameplayStatics::ApplyDamage(TargetPlayer, Damage, GetController(), this, UDamageType::StaticClass());
	}
	else
	{
		// Out of range -> resume chase
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

	if (Health <= 0.f)
	{
		bIsDead = true;
		StopAttack();
		StopChasingTarget();

	
		//stäng av collision vid ragdoll (death) osäker hur vi vill göra sen dokc
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		BP_OnDeath();
		// Optional: auto-destroy
		SetLifeSpan(5.f);
	}
	return Actual;
}
