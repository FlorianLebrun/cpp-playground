public capture_current_context_registers

.code

; void capture_current_context_registers(CONTEXT& context: rax)
capture_current_context_registers:

mov qword ptr [rax+98h],rsp
mov qword ptr [rax+0F8h],rsp
ret

end