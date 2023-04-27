# type: ignore
import collections
import sys
import typing

try:
    from typing_extensions import _AnnotatedAlias
except Exception:
    try:
        from typing import _AnnotatedAlias
    except Exception:
        _AnnotatedAlias = None

try:
    from typing_extensions import get_type_hints as _get_type_hints
except Exception:
    from typing import get_type_hints as _get_type_hints

try:
    from typing_extensions import NotRequired, Required
except Exception:
    try:
        from typing import NotRequired, Required
    except Exception:
        Required = NotRequired = None


if Required is None and _AnnotatedAlias is None:
    # No extras available, so no `include_extras`
    get_type_hints = _get_type_hints
else:

    def get_type_hints(obj):
        return _get_type_hints(obj, include_extras=True)


# The `is_class` argument was new in 3.11, but was backported to 3.9 and 3.10.
# It's _likely_ to be available for 3.9/3.10, but may not be. Easiest way to
# check is to try it and see. This check can be removed when we drop support
# for Python 3.10.
try:
    typing.ForwardRef("Foo", is_class=True)
except TypeError:

    def _forward_ref(value):
        return typing.ForwardRef(value, is_argument=False)

else:

    def _forward_ref(value):
        return typing.ForwardRef(value, is_argument=False, is_class=True)


def _apply_params(obj, mapping):
    if params := getattr(obj, "__parameters__", None):
        args = tuple(mapping.get(p, p) for p in params)
        return obj[args]
    elif isinstance(obj, typing.TypeVar):
        return mapping.get(obj, obj)
    return obj


def _get_class_mro_and_typevar_mappings(obj):
    mapping = {}

    if isinstance(obj, type):
        cls = obj
    else:
        cls = obj.__origin__

    def inner(c, scope):
        if isinstance(c, type):
            cls = c
            new_scope = {}
        else:
            cls = c.__origin__
            if cls in (object, typing.Generic):
                return
            if cls not in mapping:
                params = cls.__parameters__
                args = tuple(_apply_params(a, scope) for a in c.__args__)
                assert len(params) == len(args)
                mapping[cls] = new_scope = dict(zip(params, args))

        if issubclass(cls, typing.Generic):
            bases = getattr(cls, "__orig_bases__", cls.__bases__)
            for b in bases:
                inner(b, new_scope)

    inner(obj, {})
    return cls.__mro__, mapping


def get_class_annotations(obj):
    """Get the annotations for a class.

    This is similar to ``typing.get_type_hints``, except:

    - We maintain it
    - It leaves extras like ``Annotated``/``ClassVar`` alone
    - It resolves any parametrized generics in the class mro. The returned
      mapping may still include ``TypeVar`` values, but those should be treated
      as their unparametrized variants (i.e. equal to ``Any`` for the common case).

    Note that this function doesn't check that Generic types are being used
    properly - invalid uses of `Generic` may slip through without complaint.

    The assumption here is that the user is making use of a static analysis
    tool like ``mypy``/``pyright`` already, which would catch misuse of these
    APIs.
    """
    hints = {}
    mro, typevar_mappings = _get_class_mro_and_typevar_mappings(obj)

    for cls in mro:
        if cls in (typing.Generic, object):
            continue

        mapping = typevar_mappings.get(cls)
        cls_locals = dict(vars(cls))
        cls_globals = getattr(sys.modules.get(cls.__module__, None), "__dict__", {})

        ann = cls.__dict__.get("__annotations__", {})
        for name, value in ann.items():
            if name in hints:
                continue
            if value is None:
                value = type(None)
            elif isinstance(value, str):
                value = _forward_ref(value)
            value = typing._eval_type(value, cls_locals, cls_globals)
            if mapping is not None:
                value = _apply_params(value, mapping)
            hints[name] = value
    return hints


# A mapping from a type annotation (or annotation __origin__) to the concrete
# python type that msgspec will use when decoding. THIS IS PRIVATE FOR A
# REASON. DON'T MUCK WITH THIS.
_CONCRETE_TYPES = {
    list: list,
    tuple: tuple,
    set: set,
    frozenset: frozenset,
    dict: dict,
    typing.List: list,
    typing.Tuple: tuple,
    typing.Set: set,
    typing.FrozenSet: frozenset,
    typing.Dict: dict,
    typing.Collection: list,
    typing.MutableSequence: list,
    typing.Sequence: list,
    typing.MutableMapping: dict,
    typing.Mapping: dict,
    typing.MutableSet: set,
    typing.AbstractSet: set,
    collections.abc.Collection: list,
    collections.abc.MutableSequence: list,
    collections.abc.Sequence: list,
    collections.abc.MutableSet: set,
    collections.abc.Set: set,
    collections.abc.MutableMapping: dict,
    collections.abc.Mapping: dict,
}


def get_typeddict_hints(obj):
    """Same as `get_type_hints`, but strips off Required/NotRequired"""
    hints = get_type_hints(obj)
    out = {}
    for k, v in hints.items():
        # Strip off Required/NotRequired
        if getattr(v, "__origin__", False) in (Required, NotRequired):
            v = v.__args__[0]
        out[k] = v
    return out


def get_dataclass_info(cls):
    required = []
    optional = []
    defaults = []
    hints = None

    if hasattr(cls, "__dataclass_fields__"):
        from dataclasses import _FIELD, _FIELD_INITVAR, MISSING

        for field in cls.__dataclass_fields__.values():
            if field._field_type is not _FIELD:
                if field._field_type is _FIELD_INITVAR:
                    raise TypeError(
                        "dataclasses with `InitVar` fields are not supported"
                    )
                continue
            name = field.name
            typ = field.type
            if type(typ) is str:
                if hints is None:
                    hints = get_type_hints(cls)
                typ = hints[name]
            if field.default is not MISSING:
                defaults.append(field.default)
                optional.append((name, typ, False))
            elif field.default_factory is not MISSING:
                defaults.append(field.default_factory)
                optional.append((name, typ, True))
            else:
                required.append((name, typ, False))

        required.extend(optional)

        pre_init = None
        post_init = getattr(cls, "__post_init__", None)
    else:
        from attrs import NOTHING, Factory

        for field in cls.__attrs_attrs__:
            name = field.name
            typ = field.type
            default = field.default
            if type(typ) is str:
                if hints is None:
                    hints = get_type_hints(cls)
                typ = hints[name]
            if default is not NOTHING:
                if isinstance(default, Factory):
                    if default.takes_self:
                        raise NotImplementedError(
                            "Support for default factories with `takes_self=True` "
                            "is not implemented. "
                            "File a GitHub issue if you need "
                            "this feature!"
                        )
                    defaults.append(default.factory)
                    optional.append((name, typ, True))
                else:
                    defaults.append(default)
                    optional.append((name, typ, False))
            else:
                required.append((name, typ, False))

        required.extend(optional)

        pre_init = getattr(cls, "__attrs_pre_init__", None)
        post_init = getattr(cls, "__attrs_post_init__", None)

    return tuple(required), tuple(defaults), pre_init, post_init


def rebuild(cls, kwargs):
    """Used to unpickle Structs with keyword-only fields"""
    return cls(**kwargs)