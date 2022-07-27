bool motor_is_drive_scheduled();
void motor_schedule_drive(uint32_t delay);
void motor_cancel_drive();
int64_t start_drive_motor(alarm_id_t id, void *user_data);
int64_t stop_drive_motor(alarm_id_t id, void *user_data);

