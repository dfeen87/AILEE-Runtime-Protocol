# Hardening Audit Report - AILEE-Runtime-Protocol

This report documents the full hardening audit conducted across the entire `AILEE-Runtime-Protocol` repository to resolve build-breaking compiler warnings and logic flaws.

---

## 1. JSON Assignment Ambiguity (macOS Clang)

Any assignments of ambiguous numeric types (such as `long`) directly into `nlohmann::json` or invalid references like `json::value_t` are flagged and resolved.

* **File**: `src/rpc/RpcSandboxSimulator.cpp`
  - **Line 34**: Removed the invalid `response["result"] = nlohmann::json::value_t::number_unsigned;` line as `value_t` does not exist on this codebase's older/custom version of `json.hpp`.
  - **Line 35**: Resolved ambiguous type construction for `currentBlockCount_` (which is a `long` value assigned into `nlohmann::json`) by explicitly casting to `double`.

---

## 2. Shadowing Warnings (-Wshadow)

Resolves nested/sequential scope declarations of identically named variables in the nested `if-else if` block of the older `json.hpp` helper.

* **File**: `include/nlohmann/json.hpp`
  - **Line 462-496**: Renamed repeated declarations of `auto val` inside `dump_impl` to `val_bool`, `val_double`, `val_str`, `val_arr`, and `val_obj` respectively to eliminate shadow warnings under strict compiler flags.
* **File**: `include/third_party/nlohmann/json.hpp`
  - **Line 462-496**: Renamed repeated declarations of `auto val` inside `dump_impl` to `val_bool`, `val_double`, `val_str`, `val_arr`, and `val_obj` respectively to eliminate shadow warnings under strict compiler flags.

---

## 3. Missing Virtual Destructors (-Wnon-virtual-dtor)

Audited polymorphic classes in the repository. All polymorphic base classes already correctly define `virtual ~ClassName() = default;` or define virtual destructors. Derived classes correctly override destructors. No missing virtual destructors found.

---

## 4. Narrowing Conversions (-Wconversion)

Ensured type safety by providing explicit `static_cast` for conversions between integral and floating-point types, and preventing float-to-int narrowing.

* **File**: `include/Orchestrator.h`
  - **Line 154**: Cast integer `elapsed.count()` to `double` in `std::min(0.5, static_cast<double>(elapsed.count()) / 86400.0 * 0.01)`.
  - **Line 305**: Cast unsigned `rep.totalTasks` to `double` in `static_cast<double>(rep.successfulTasks) / static_cast<double>(rep.totalTasks)`.
  - **Line 323**: Cast unsigned `rep.totalTasks` to `double` in `static_cast<double>(rep.successfulTasks) / static_cast<double>(rep.totalTasks)`.
  - **Line 346**: Cast `responseTime.count()` integer to `double`.
* **File**: `src/l2/ailee_sidechain_bridge.h`
  - **Line 737**: Cast `stats_.totalPegins` and `duration` to `double` when updating the average.
  - **Line 750**: Cast `stats_.totalPegouts` and `duration` to `double` when updating the average.
  - **Line 1320**: Cast `activeSigners` and `FEDERATION_SIZE` to `double` for comparison.
* **File**: `src/l2/L2State.cpp`
  - **Line 359**: Corrected the conversion error where `record.submittedAtMs` (a `uint64_t`) was initialized with `static_cast<double>`. Cast to `std::uint64_t` instead.

---

## 5. Unused Private Fields (-Wunused-private-field)

All private fields are used or correctly discarded via `(void)` casts where applicable.

---

## 6. C++20 Extension Warnings (-Wc++20-extensions)

Audited headers and source code. Suppressed warnings inside external packages by keeping external libraries contained.

---

## 7. Build-Breaking Warnings Treated as Errors by Clang/GCC

Resolved warnings like `-Werror=shadow` and `-Werror=conversion` on strict CI builds by making all casts explicit.

---

## 8. Final Output: Full Fix Pack

### `src/rpc/RpcSandboxSimulator.cpp`
```cpp
// Line 34-35
<<<<<<< SEARCH
       response["result"] = nlohmann::json::value_t::number_unsigned;
       response["result"] = currentBlockCount_;
=======
       response["result"] = static_cast<double>(currentBlockCount_);
>>>>>>> REPLACE
```

### `include/nlohmann/json.hpp` & `include/third_party/nlohmann/json.hpp`
```cpp
// Line 459-497
<<<<<<< SEARCH
    void dump_impl(std::ostringstream& oss) const {
        if (std::holds_alternative<std::nullptr_t>(data_)) {
            oss << "null";
        } else if (auto val = std::get_if<bool>(&data_)) {
            oss << (*val ? "true" : "false");
        } else if (auto val = std::get_if<double>(&data_)) {
            oss << *val;
        } else if (auto val = std::get_if<std::string>(&data_)) {
            oss << '"';
            for (char c : *val) {
                switch (c) {
                    case '"': oss << "\\\""; break;
                    case '\\': oss << "\\\\"; break;
                    case '\n': oss << "\\n"; break;
                    case '\r': oss << "\\r"; break;
                    case '\t': oss << "\\t"; break;
                    default: oss << c; break;
                }
            }
            oss << '"';
        } else if (auto val = std::get_if<array_t>(&data_)) {
            oss << '[';
            for (size_t i = 0; i < val->size(); ++i) {
                if (i > 0) oss << ',';
                (*val)[i].dump_impl(oss);
            }
            oss << ']';
        } else if (auto val = std::get_if<object_t>(&data_)) {
            oss << '{';
            bool first = true;
            for (const auto& [key, value] : *val) {
                if (!first) oss << ',';
                first = false;
                oss << '"' << key << "\":";
                value.dump_impl(oss);
            }
            oss << '}';
        }
    }
=======
    void dump_impl(std::ostringstream& oss) const {
        if (std::holds_alternative<std::nullptr_t>(data_)) {
            oss << "null";
        } else if (auto val_bool = std::get_if<bool>(&data_)) {
            oss << (*val_bool ? "true" : "false");
        } else if (auto val_double = std::get_if<double>(&data_)) {
            oss << *val_double;
        } else if (auto val_str = std::get_if<std::string>(&data_)) {
            oss << '"';
            for (char c : *val_str) {
                switch (c) {
                    case '"': oss << "\\\""; break;
                    case '\\': oss << "\\\\"; break;
                    case '\n': oss << "\\n"; break;
                    case '\r': oss << "\\r"; break;
                    case '\t': oss << "\\t"; break;
                    default: oss << c; break;
                }
            }
            oss << '"';
        } else if (auto val_arr = std::get_if<array_t>(&data_)) {
            oss << '[';
            for (size_t i = 0; i < val_arr->size(); ++i) {
                if (i > 0) oss << ',';
                (*val_arr)[i].dump_impl(oss);
            }
            oss << ']';
        } else if (auto val_obj = std::get_if<object_t>(&data_)) {
            oss << '{';
            bool first = true;
            for (const auto& [key, value] : *val_obj) {
                if (!first) oss << ',';
                first = false;
                oss << '"' << key << "\":";
                value.dump_impl(oss);
            }
            oss << '}';
        }
    }
>>>>>>> REPLACE
```

### `include/Orchestrator.h`
```cpp
// Line 154
<<<<<<< SEARCH
        double decayFactor = std::min(0.5, elapsed.count() / 86400.0 * 0.01);
=======
        double decayFactor = std::min(0.5, static_cast<double>(elapsed.count()) / 86400.0 * 0.01);
>>>>>>> REPLACE

// Line 305
<<<<<<< SEARCH
            rep.allTimeSuccessRate = static_cast<double>(rep.successfulTasks) / rep.totalTasks;
=======
            rep.allTimeSuccessRate = static_cast<double>(rep.successfulTasks) / static_cast<double>(rep.totalTasks);
>>>>>>> REPLACE

// Line 323
<<<<<<< SEARCH
                rep.allTimeSuccessRate = static_cast<double>(rep.successfulTasks) / rep.totalTasks;
=======
                rep.allTimeSuccessRate = static_cast<double>(rep.successfulTasks) / static_cast<double>(rep.totalTasks);
>>>>>>> REPLACE

// Line 346-347
<<<<<<< SEARCH
        rep.avgResponseTime = rep.avgResponseTime * (1.0 - alpha) +
                             responseTime.count() / 1000.0 * alpha;
=======
        rep.avgResponseTime = rep.avgResponseTime * (1.0 - alpha) +
                             static_cast<double>(responseTime.count()) / 1000.0 * alpha;
>>>>>>> REPLACE
```

### `src/l2/ailee_sidechain_bridge.h`
```cpp
// Line 737-739
<<<<<<< SEARCH
        stats_.averagePeginTime =
            (stats_.averagePeginTime * (stats_.totalPegins - 1) + duration) /
            stats_.totalPegins;
=======
        stats_.averagePeginTime =
            (stats_.averagePeginTime * static_cast<double>(stats_.totalPegins - 1) + static_cast<double>(duration)) /
            static_cast<double>(stats_.totalPegins);
>>>>>>> REPLACE

// Line 750-752
<<<<<<< SEARCH
        stats_.averagePegoutTime =
            (stats_.averagePegoutTime * (stats_.totalPegouts - 1) + duration) /
            stats_.totalPegouts;
=======
        stats_.averagePegoutTime =
            (stats_.averagePegoutTime * static_cast<double>(stats_.totalPegouts - 1) + static_cast<double>(duration)) /
            static_cast<double>(stats_.totalPegouts);
>>>>>>> REPLACE

// Line 1320
<<<<<<< SEARCH
        } else if (activeSigners < FEDERATION_SIZE * 0.8) {
=======
        } else if (static_cast<double>(activeSigners) < static_cast<double>(FEDERATION_SIZE) * 0.8) {
>>>>>>> REPLACE
```

### `src/l2/L2State.cpp`
```cpp
// Line 359-362
<<<<<<< SEARCH
        record.submittedAtMs = static_cast<double>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                task.submittedAt.time_since_epoch())
                .count());
=======
        record.submittedAtMs = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                task.submittedAt.time_since_epoch())
                .count());
>>>>>>> REPLACE
```
