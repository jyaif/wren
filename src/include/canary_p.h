
#ifndef CANARY_P_H
#define CANARY_P_H

#ifndef CANARY_H
#error "Do not include directly. #include \"canary.h\" instead."
#endif // CANARY_H

static inline bool
canary_is_slot_type(canary_context_t *context, canary_slot_t slot,
                    canary_type_t type) {
  return canary_get_slot_type(context, slot) == type;
}

#endif // CANARY_P_H
