/* SPDX-License-Identifier: MIT
 * Supplement opaque original state with persistent, readable native controls.
 * Some original modules omit controls (e.g. Hera volume) from their restore path.
 * This runs only at the existing host's state boundary, never during render.
 * GMV1 is private to these new adapters; legacy raw original chunks still load.
 */
static intptr_t move_save(wrap_t *w, void *out) {
    const size_t offset = 12 + 4 * NPARAMS;
    if (offset >= sizeof w->chunk) return 0;
    char *raw = w->chunk + offset;
    const size_t available = sizeof w->chunk - offset;
    memset(raw, 0, available);
    int n = eng_get(w, "state", raw, (int)available);
    if (n < 0 || (size_t)n >= available) return 0;
    const uint32_t length = n > 0 ? (uint32_t)strnlen(raw, available) + 1 : 1;
    memcpy(w->chunk, "GMV1", 4);
    const uint32_t count = NPARAMS;
    memcpy(w->chunk + 4, &count, 4);
    memcpy(w->chunk + 8, &length, 4);
    for (unsigned i = 0; i < count; ++i) {
        float value = NAN;
        char text[256] = {0};
        const param_t *p = &PARAMS[i];
        if (!p->momentary && !p->string_display && p->popup_of < 0 && p->step_target < 0
            && eng_get(w, p->key, text, sizeof text) > 0) {
            /* A negative preset identifies an edited patch, not preset zero. */
            if (!((strstr(p->key,"preset")||strstr(p->key,"groove_model_")) && atof(text)<0)) value = str_to_norm(p, text);
        }
        memcpy(w->chunk + 12 + 4*i, &value, 4);
    }
    *(void **)out = w->chunk;
    return (intptr_t)(offset + length);
}
static intptr_t move_restore(wrap_t *w, const void *data, size_t size) {
    if (!data || !size || size > sizeof w->chunk) return 0;
    if (size < 4 || memcmp(data, "GMV1", 4)) {
        memcpy(w->chunk, data, size);w->chunk[size-1] = 0;
        eng_set(w, "state", w->chunk);return 1;
    }
    if (size < 12) return 0;
    uint32_t count, length;
    memcpy(&count, (const char *)data + 4, 4);
    memcpy(&length, (const char *)data + 8, 4);
    if (count != NPARAMS || !length || size != 12 + 4*(size_t)count + length
        || ((const char *)data)[size-1]) return 0;
    memcpy(w->chunk, data, size);
    eng_set(w, "state", w->chunk + 12 + 4*count);
    /* Selectors precede their edited controls. Do not reselect an unchanged preset. */
    for (int pass = 0; pass < 2; ++pass) for (unsigned i = 0; i < count; ++i) {
        const param_t *p = &PARAMS[i];
        int selector = strstr(p->key, "preset") || strstr(p->key, "program")
            || !strcmp(p->key, "machine") || !strcmp(p->key, "kit");
        if (selector != (pass == 0) || p->momentary || p->string_display
            || p->popup_of >= 0 || p->step_target >= 0) continue;
        float value;memcpy(&value, w->chunk + 12 + 4*i, 4);
        if (!isfinite(value) || value < 0 || value > 1) continue;
        char text[256] = {0};
        if (eng_get(w, p->key, text, sizeof text) > 0
            && fabsf(str_to_norm(p, text) - value) < 0.000001f) continue;
        norm_to_str(p, value, text, sizeof text);eng_set(w, p->key, text);
    }
    return 1;
}
