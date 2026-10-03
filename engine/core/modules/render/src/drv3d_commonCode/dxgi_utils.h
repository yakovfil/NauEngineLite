// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#if _TARGET_PC_WIN

    #include <optional>
    #include <utility>

    #include "nau/string/string.h"
    #include "nau/supp/dag_comPtr.h"

struct IDXGIAdapter;
struct IDXGIOutput;

nau::string get_monitor_name_from_output(IDXGIOutput *pOutput);

/* Returns with the monitor's ptr if it exists with the given name,
 otherwise returns with nullptr.
 All parameters must be valid pointers. */
ComPtr<IDXGIOutput> get_output_monitor_by_name(IDXGIAdapter *pAdapter, const char *monitorName);

/* Finds a monitor with the given name and returns with that.
 if isn't exist return with the primary monitor's pointer if it's available. */
ComPtr<IDXGIOutput> get_output_monitor_by_name_or_default(IDXGIAdapter *pAdapter, const char *monitorName = nullptr);

/* Returns with the primary monitor's pointer if it's available. */
ComPtr<IDXGIOutput> get_default_monitor(IDXGIAdapter *pAdapter);

bool resolutions_have_same_ratio(std::pair<uint32_t, uint32_t> l_res, std::pair<uint32_t, uint32_t> r_res);

std::optional<std::pair<uint32_t, uint32_t>> get_recommended_resolution(IDXGIOutput* dxgi_output);

#endif
