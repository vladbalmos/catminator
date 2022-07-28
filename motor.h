void motor_init_pins(uint upin, uint dpin);
bool motor_is_drive_scheduled();
bool motor_is_running();
void motor_schedule_drive(uint delay);
void motor_cancel_drive();
int64_t start_drive_motor(alarm_id_t id, void *user_data);
int64_t stop_drive_motor(alarm_id_t id, void *user_data);

