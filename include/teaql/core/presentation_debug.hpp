#pragma once
#include <teaql/core/debug.hpp>
#include <teaql/core/web.hpp>
#include <teaql/core/xls.hpp>

namespace teaql {
// Raw presentation-model diagnostics; unrelated to installing a web framework.
inline void write_debug(DebugWriter&, const WebStyle&);
inline void write_debug(DebugWriter&, const WebAction&);
inline void write_debug(DebugWriter&, const WebResponse&);
inline void write_debug(DebugWriter&, const XlsBlock&);
inline void write_debug(DebugWriter&, const XlsBlockBuildContext&);
inline void write_debug(DebugWriter&, const XlsPage&);
inline void write_debug(DebugWriter&, const XlsWorkbook&);

inline void write_debug(DebugWriter& writer, const WebStyle& value) {
    writer.structure("WebStyle", [&](auto& fields) {
        fields.field("background_color", [&](auto& out) {
            if (value.background_color) {
                out.tuple("Some", [&](auto& inner) {
                    write_debug(inner, *value.background_color);
                });
            } else {
                out.raw("None");
            }
        });
        fields.field("color", [&](auto& out) {
            if (value.color) {
                out.tuple("Some", [&](auto& inner) {
                    write_debug(inner, *value.color);
                });
            } else {
                out.raw("None");
            }
        });
        fields.field("class_names", [&](auto& out) {
            if (value.class_names) {
                out.tuple("Some", [&](auto& inner) {
                    write_debug(inner, *value.class_names);
                });
            } else {
                out.raw("None");
            }
        });
    });
}

inline void write_debug(DebugWriter& writer, const WebAction& value) {
    writer.structure("WebAction", [&](auto& fields) {
        fields.field("key", [&](auto& out) {
            if (value.key) {
                out.tuple("Some", [&](auto& inner) {
                    write_debug(inner, *value.key);
                });
            } else {
                out.raw("None");
            }
        });
        fields.field("name", [&](auto& out) {
            if (value.name) {
                out.tuple("Some", [&](auto& inner) {
                    write_debug(inner, *value.name);
                });
            } else {
                out.raw("None");
            }
        });
        fields.field("level", [&](auto& out) {
            if (value.level) {
                out.tuple("Some", [&](auto& inner) {
                    write_debug(inner, *value.level);
                });
            } else {
                out.raw("None");
            }
        });
        fields.field("execute", [&](auto& out) {
            if (value.execute) {
                out.tuple("Some", [&](auto& inner) {
                    write_debug(inner, *value.execute);
                });
            } else {
                out.raw("None");
            }
        });
        fields.field("target", [&](auto& out) {
            if (value.target) {
                out.tuple("Some", [&](auto& inner) {
                    write_debug(inner, *value.target);
                });
            } else {
                out.raw("None");
            }
        });
        fields.field("component", [&](auto& out) {
            if (value.component) {
                out.tuple("Some", [&](auto& inner) {
                    write_debug(inner, *value.component);
                });
            } else {
                out.raw("None");
            }
        });
        fields.field("warning_message", [&](auto& out) {
            if (value.warning_message) {
                out.tuple("Some", [&](auto& inner) {
                    write_debug(inner, *value.warning_message);
                });
            } else {
                out.raw("None");
            }
        });
        fields.field("role_for_list", [&](auto& out) {
            if (value.role_for_list) {
                out.tuple("Some", [&](auto& inner) {
                    write_debug(inner, *value.role_for_list);
                });
            } else {
                out.raw("None");
            }
        });
        fields.field("request_url", [&](auto& out) {
            if (value.request_url) {
                out.tuple("Some", [&](auto& inner) {
                    write_debug(inner, *value.request_url);
                });
            } else {
                out.raw("None");
            }
        });
    });
}

inline void write_debug(DebugWriter& writer, const WebResponse& value) {
    writer.structure("WebResponse", [&](auto& fields) {
        fields.field("data", [&](auto& out) {
            out.sequence(value.data, DefaultDebugElement{});
        });
        fields.field("result_code", [&](auto& out) {
            write_debug(out, value.result_code);
        });
        fields.field("status", [&](auto& out) {
            if (value.status) {
                out.tuple("Some", [&](auto& inner) {
                    write_debug(inner, *value.status);
                });
            } else {
                out.raw("None");
            }
        });
        fields.field("message", [&](auto& out) {
            if (value.message) {
                out.tuple("Some", [&](auto& inner) {
                    write_debug(inner, *value.message);
                });
            } else {
                out.raw("None");
            }
        });
        fields.field("record_count", [&](auto& out) {
            write_debug(out, value.record_count);
        });
        fields.field("version", [&](auto& out) {
            write_debug(out, value.version);
        });
        fields.field("facets", [&](auto& out) {
            out.sequence(
                value.facets,
                [](auto& inner, const auto& pair) {
                    inner.text(pair.first);
                    inner.raw(": ");
                    write_debug(inner, pair.second);
                },
                "{",
                "}");
        });
    });
}

inline void write_debug(DebugWriter& writer, const XlsBlock& value) {
    writer.structure("XlsBlock", [&](auto& fields) {
        fields.field("page", [&](auto& out) {
            write_debug(out, value.page);
        });
        fields.field("top", [&](auto& out) {
            write_debug(out, value.top);
        });
        fields.field("bottom", [&](auto& out) {
            write_debug(out, value.bottom);
        });
        fields.field("left", [&](auto& out) {
            write_debug(out, value.left);
        });
        fields.field("right", [&](auto& out) {
            write_debug(out, value.right);
        });
        fields.field("style_refer_block", [&](auto& out) {
            if (value.style_refer_block) {
                out.tuple("Some", [&](auto& inner) {
                    write_debug(inner, *value.style_refer_block);
                });
            } else {
                out.raw("None");
            }
        });
        fields.field("value", [&](auto& out) {
            write_debug(out, value.value);
        });
        fields.field("properties", [&](auto& out) {
            out.sequence(
                value.properties,
                [](auto& inner, const auto& pair) {
                    inner.text(pair.first);
                    inner.raw(": ");
                    write_debug(inner, pair.second);
                },
                "{",
                "}");
        });
    });
}

inline void write_debug(DebugWriter& writer, const XlsBlockBuildContext& value) {
    writer.structure("XlsBlockBuildContext", [&](auto& fields) {
        fields.field("page", [&](auto& out) {
            write_debug(out, value.page);
        });
        fields.field("start_x", [&](auto& out) {
            write_debug(out, value.start_x);
        });
        fields.field("x", [&](auto& out) {
            write_debug(out, value.x);
        });
        fields.field("y", [&](auto& out) {
            write_debug(out, value.y);
        });
    });
}

inline void write_debug(DebugWriter& writer, const XlsPage& value) {
    writer.structure("XlsPage", [&](auto& fields) {
        fields.field("name", [&](auto& out) {
            write_debug(out, value.name);
        });
        fields.field("blocks", [&](auto& out) {
            out.sequence(value.blocks, DefaultDebugElement{});
        });
    });
}

inline void write_debug(DebugWriter& writer, const XlsWorkbook& value) {
    writer.structure("XlsWorkbook", [&](auto& fields) {
        fields.field("pages", [&](auto& out) {
            out.sequence(value.pages, DefaultDebugElement{});
        });
    });
}

}
