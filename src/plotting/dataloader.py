import yaml

def loadFromYamlPath(path):
    with open(path) as f:
        docs = list(yaml.safe_load_all(f))  # allow multi-doc

    # Flatten: accept lists or single maps
    out = []
    for d in docs:
        if d is None:
            continue
        if isinstance(d, list):
            out.extend(d)
        else:
            # if someone accidentally dumped a single map, keep it
            out.append(d)
    return out