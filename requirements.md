# JR-QRMaker Requirements

## Purpose

A simple desktop app that turns text or a link into a QR code SVG file.

## Requirements

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

## issue

- [ ] **ticket-001** — too long text crasing aplication.

## How to check

1. Copy a link or text to the clipboard and open the app.
2. Click **Paste**. Confirm the QR content field shows the copied text.
3. Click within the QR content field, use Left/Right to move the cursor, and edit the text. Confirm Enter does not insert a new line.
4. Click the output file name field, use Left/Right (and Home/End) to move the cursor, and edit the name. Confirm Enter does not insert a new line.
5. Click **Generate**.
6. Confirm the SVG file was created with the edited name and its QR code contains the text shown in the QR content field.
