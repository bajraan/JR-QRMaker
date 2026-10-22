## Wymagania Systemowe (System Requirements)

- [x] SRQ-STYLE-01: Obsługa kształtów modułów
* **Description:** Moduł `SvgWriter` musi wspierać renderowanie modułów kodu QR jako koła (`<circle>`) oraz kwadraty (`<rect>`).
* **User Requirements:** URQ-01
* **Test:** TT-1000
* **Status:** Zaimplementowane
- [x] SRQ-STYLE-02: Kolorowanie segmentów QR
* **Opis:** System musi pozwalać na niezależne przypisanie kolorów w formacie HEX dla korpusu (body), zewnętrznych ramek oczu oraz środkowych punktów oczu.
* **Realizuje wymaganie:** URQ-01
* **Stan:** Zaimplementowane

=== OLD === OLD ===

# JR-QRMaker Requirements

## Requirements -

- [x] **REQ-001 — Enter QR content:** The user can enter or paste a link or text into the GUI text field.

- [x] **REQ-002 — Paste from clipboard:** A **Paste** button copies the current clipboard text into the QR content field.

- [x] **REQ-003 — Show QR content:** The QR content field displays the actual text that will be encoded in the QR code.

- [x] **REQ-004 — Set output file name:** The output file name field displays the name of the SVG file that will be created.

- [x] **REQ-005 — Edit output file name field:** The user can edit the output file name before generating the QR code.

- [x] **REQ-006 — One-line output name:** The output file name field accepts one line of text.

- [x] **REQ-007 — Move output name cursor:** Left/Right arrows move the cursor in the output file name field; Home/End move it to either end.

- [x] **REQ-008 — Generate QR code:** The user can click a **Generate** button to create a QR code from the content field.

- [x] **REQ-009 — One-line QR content:** The QR content field accepts one line of text.

- [x] **REQ-010 — Edit QR content:** The QR content field is editable, including insertion at the cursor position.

- [x] **REQ-011 — Save SVG file:** Generating the QR code saves it as an SVG file using the name in the output file name field and shows where it was saved.

## system level

- [x] **SYS-001** — Application Layout PIC1

## issue

- [ ] **ticket-001** — too long text crasing aplication.
