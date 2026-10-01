#pragma once

enum class IntakeState { OFF, IN, OUT };
extern IntakeState intakeState;

void manualIntake();
void setIntakeState(IntakeState state);
void applyIntakeState();
