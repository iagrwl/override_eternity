#pragma once

enum class IntakeState { OFF, IN, OUT };

void manualIntake();
void setIntakeState(IntakeState state);
void applyIntakeState();
