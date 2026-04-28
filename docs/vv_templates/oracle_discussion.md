
## Thread 1 — Oracle versioning (preventing "oracle drift")

Each oracle class has a different fragility profile:

| Class | Drift risk | What can silently change |
|---|---|---|
| 1 Analytical | None | Math is math |
| 2 Reference impl | **High** | MTEX/SciPy/NumPy version, random seed, default dtype, parallelism nondeterminism |
| 3 Paper-based | Low–Medium | Errata, revised editions, supplementary code that evolves |
| 4 Invariant | None | Invariants are properties, not numbers |
| 5 Expert-visual | **High** (social) | Expert changes their mind, leaves, or forgets what "approved" meant |

**Recommended policy per class:**

- **Class 2:** archive must pin *library name + exact version + seed (if any) + the script itself*. Example record: `MTEX 5.9.0 (released 2023-05-14), Octave 8.2.0, seed = 42, script: orcl_mtex.m`. If you want ongoing reproducibility, also commit a hash of the script's output so a future engineer can re-run and detect drift without reading numbers.
- **Class 3:** archive must pin *DOI + edition + figure/table/equation number + page number*. Strongly recommend embedding a copy of the paper (or the specific figure) in the archive, because journal paywalls and URL rot are real.
- **Class 5:** archive must contain *named expert + date + signed-off screenshot(s) of the approved output*. This is the load-bearing artifact when the expert isn't around three years later.

Classes 1 and 4 need no versioning — the oracle is the math or the property, both of which live in the test code directly.

**Concrete proposal:** add a short "Oracle provenance" block to the archive ReadMe, required for class 2, 3, and 5 oracles. One paragraph. Engineers tend to write too much here; a tight template helps.

---

## Thread 2 — Where each oracle class lives

Natural homes, based on what needs to be reproducible vs. what needs to be assertable:

| Class | Lives in | Form |
|---|---|---|
| 1 Analytical | Unit test code | `REQUIRE(result == X)` with a comment showing the hand derivation |
| 2 Reference impl | Verification archive (script) + unit test (cached exemplar `.dream3d`) | Script generates the exemplar once; test compares SIMPLNX output against the cached exemplar |
| 3 Paper-based | Unit test code (numeric assertions) + archive (paper reference) | Expected numbers in `REQUIRE`s with citation comment; DOI and figure number in archive ReadMe |
| 4 Invariant | Unit test code | `REQUIRE(feature_ids.min() == 1); REQUIRE(is_contiguous(feature_ids));` |
| 5 Expert-visual | Verification archive (screenshots + sign-off) + unit test (cached exemplar `.dream3d`) | Screenshots are the oracle record; test compares against a cached exemplar the expert approved |

Useful observation: **classes 1, 3, 4 produce inline test-code assertions; classes 2 and 5 produce cached exemplar files.** This is a clean split that maps onto the existing simplnx test pattern — the "exemplar `.dream3d` file downloaded from the Data_Archive" is exactly the right mechanism for Class 2 and 5, and it already exists. The oracle policy just upgrades the provenance story for how that file earns its status.

**Open question for Mike:** want me to fold the provenance block + the class-to-location mapping into the doc? Would go under Step 0 as two subsections ("Where each oracle lives" and "Oracle provenance record"). Relatively short additions.

---

## Retroactive V&V for ~15 filters

The "supposedly" in your message is doing work — it says these aren't unverified, they're informally-verified, and the goal is to produce proper V&V artifacts. That's a different workflow from forward verification, and worth a dedicated section in the policy. Key design questions before you pick up the 15:

1. **What's the deliverable per filter?** A full archive with oracle + toy data + comparison report + Deviation entries, or just the user-facing Deviation write-ups for the website and SBIR? Different amounts of work by an order of magnitude.
2. **Is there existing work product to promote?** If a filter already has hand-verified exemplars, a paper reference in comments, or prior comparison notes, those should be retrofit-promoted into the new format rather than redone from scratch.
3. **Triage order.** Probably: (a) filters cited in SBIR deliverables; (b) filters with known user complaints about 6.5.172 divergence; (c) filters with cheap oracles (class 1 or 4) for quick wins; (d) everything else. If you can share the list, I can help classify oracle class per filter so you know where the expensive ones are before committing.
4. **Skip-criteria.** Are there filters on the list where the honest answer is "Class 5 only, no independent oracle feasible"? Those should be named now so the retrospective effort doesn't spin trying to invent oracles that don't exist.
5. **Parallelization.** 15 filters × (oracle + toy data + SIMPLNX run + 6.5.172 run + comparison + Deviation write-up) is weeks-to-months of work even at a fast clip. Is this one-person, delegated, or phased across a release cycle?

**Suggestion:** before touching the 15, add a "Retroactive Application" section to the policy that answers (1), (2), and (4) — that way the scope is bounded before anyone starts filter #1, and the SBIR-deliverable filters get prioritized explicitly rather than by accident.

**Offer:** share the list of 15 (or as much of it as makes sense) and I can sketch oracle classes against them. That's the fastest way to see whether this is a 2-week job or a 2-quarter job.

---

## Where this leaves the policy doc

The policy doc at `.claude/mtr_filter_verification_validation.md` currently has:

1. Goal
2. Step 0: Establishing Correctness — The Oracle (with 5 classes, policy, workflow a–e)
3. Legacy Comparison — Diff Explanation (Algorithm Relationship, Deviation Template, three worked examples)
4. Algorithm Review (annotated with review-algorithm skill pointers)
5. Unit Test Review (cleaned up — test-code mechanics only)
6. Documentation Updates (annotated with review-filter-docs pointers)
7. Archiving Everything When Finished (annotated with archive-filter-verification pointer)

**Open items still to resolve (from this discussion):**

- Oracle provenance block + class-to-location mapping → fold into Step 0 (pending user yes/no)
- "Retroactive Application" section → to be drafted once questions 1–5 above are answered
- Eventually: infographics addition to `review-filter-docs` skill, per earlier gap analysis
