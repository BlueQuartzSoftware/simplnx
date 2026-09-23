#!/usr/bin/env python3

import hashlib
import json
from pathlib import Path
import re
import time
import unicodedata
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen


PLUGIN_DIR = Path(__file__).resolve().parent.parent
PIPELINE_DIR = PLUGIN_DIR / "pipelines"
USER_AGENT = "DREAM3D-NX citation verification/1.0 (mailto:support@bluequartz.net)"
TIMEOUT_SECONDS = 20
MAX_ATTEMPTS = 3
ALLOWED_PAPER_TYPES = {
    "article",
    "article-journal",
    "journal-article",
    "paper-conference",
    "proceedings-article",
    "report",
}


def normalize_text(value):
    value = unicodedata.normalize("NFKD", value)
    return "".join(character.lower() for character in value if character.isalnum())


def request_bytes(url, accept):
    request = Request(url, headers={"Accept": accept, "User-Agent": USER_AGENT})
    last_error = None
    for attempt in range(MAX_ATTEMPTS):
        try:
            with urlopen(request, timeout=TIMEOUT_SECONDS) as response:
                return response.read(), response.headers.get_content_type(), response.geturl()
        except (HTTPError, URLError, TimeoutError) as error:
            last_error = error
            is_transient = isinstance(error, URLError) or getattr(error, "code", 0) in {408, 425, 429, 500, 502, 503, 504}
            if not is_transient or attempt + 1 == MAX_ATTEMPTS:
                break
            time.sleep(attempt + 1)
    raise RuntimeError(f"Could not load {url}: {last_error}")


def metadata_year(metadata):
    for key in ("issued", "published", "published-print", "published-online"):
        date_parts = metadata.get(key, {}).get("date-parts", [])
        if date_parts and date_parts[0]:
            return int(date_parts[0][0])
    raise RuntimeError("DOI metadata does not contain a publication year")


def verify_doi(citation, context):
    declared_doi = citation["doi"].lower()
    body, content_type, resolved_url = request_bytes(
        f"https://doi.org/{declared_doi}",
        "application/vnd.citationstyles.csl+json",
    )
    if content_type not in {"application/json", "application/vnd.citationstyles.csl+json"}:
        raise RuntimeError(f"{context}: DOI returned unexpected content type {content_type} from {resolved_url}")
    metadata = json.loads(body)

    returned_doi = str(metadata.get("DOI", metadata.get("doi", ""))).lower()
    if returned_doi != declared_doi:
        raise RuntimeError(f"{context}: DOI metadata returned {returned_doi!r}, expected {declared_doi!r}")

    declared_title = normalize_text(citation["title"])
    returned_title = normalize_text(metadata.get("title", ""))
    if declared_title != returned_title:
        raise RuntimeError(f"{context}: title mismatch: {metadata.get('title')!r}")

    if metadata_year(metadata) != int(citation["year"]):
        raise RuntimeError(f"{context}: publication year mismatch")

    returned_type = metadata.get("type", "")
    if returned_type not in ALLOWED_PAPER_TYPES:
        raise RuntimeError(f"{context}: DOI identifies {returned_type!r}, not a supported scientific paper type")

    returned_families = {normalize_text(author.get("family", "")) for author in metadata.get("author", [])}
    for declared_author in citation["authors"]:
        declared_parts = re.split(r"\s+", declared_author.strip())
        if normalize_text(declared_parts[-1]) in {"jr", "sr", "ii", "iii", "iv"}:
            declared_parts.pop()
        declared_family = normalize_text(declared_parts[-1])
        if not any(returned_family.endswith(declared_family) or declared_family.endswith(returned_family) for returned_family in returned_families):
            raise RuntimeError(f"{context}: author {declared_author!r} is absent from DOI metadata")


def verify_non_doi_url(citation, context):
    body, content_type, resolved_url = request_bytes(citation["url"], "application/pdf,text/html;q=0.9")
    if content_type == "application/pdf":
        if not body.startswith(b"%PDF-"):
            raise RuntimeError(f"{context}: {resolved_url} did not return a valid PDF header")
        expected_sha256 = citation.get("verification_sha256")
        if not expected_sha256:
            raise RuntimeError(f"{context}: a non-DOI PDF citation requires verification_sha256")
        actual_sha256 = hashlib.sha256(body).hexdigest()
        if actual_sha256 != expected_sha256:
            raise RuntimeError(f"{context}: PDF SHA-256 is {actual_sha256}, expected {expected_sha256}")
        return
    if content_type in {"text/html", "application/xhtml+xml"}:
        page_text = body.decode("utf-8", errors="ignore")
        title_words = [word for word in re.findall(r"[A-Za-z0-9]+", citation["title"]) if len(word) >= 5]
        if title_words and not any(word.lower() in page_text.lower() for word in title_words[:4]):
            raise RuntimeError(f"{context}: loaded HTML does not contain words from the declared paper title")
        return
    raise RuntimeError(f"{context}: {resolved_url} returned unsupported content type {content_type}")


def main():
    yaml_paths = sorted(PIPELINE_DIR.glob("([0-9][0-9]) */*.yaml"))
    if len(yaml_paths) != 16:
        raise RuntimeError(f"Expected 16 pipeline YAML files, found {len(yaml_paths)}")

    verified = set()
    for yaml_path in yaml_paths:
        metadata = json.loads(yaml_path.read_text(encoding="utf-8"))
        for index, citation in enumerate(metadata["citations"]):
            identifier = citation.get("doi") or citation["url"]
            key = identifier.lower()
            if key in verified:
                continue
            context = f"{yaml_path.name} citation {index + 1} ({citation['title']})"
            if citation.get("doi"):
                verify_doi(citation, context)
            else:
                verify_non_doi_url(citation, context)
            verified.add(key)
            print(f"PASS: {identifier} - {citation['title']}")


if __name__ == "__main__":
    main()
